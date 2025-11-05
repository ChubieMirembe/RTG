#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

// binding 1 = colour
layout(set = 0, binding = 1) uniform sampler2D colSampler;
// binding 2 = HEIGHT map now (not a normal map)
layout(set = 0, binding = 2) uniform sampler2D heightSampler;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vColor;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
    // base color
    vec3 albedo = texture(colSampler, vUV).rgb;

    // sample height (single channel)
    float h = texture(heightSampler, vUV).r;

    // screen-space derivatives of height
    float dhdx = dFdx(h);
    float dhdy = dFdy(h);

    // how “tall” the bumps look
    float bumpHeight = 1.0;   // tweak 0.1–1.0

    // build a perturbed normal in view/screen space style
    vec3 bumpN = normalize(vec3(-dhdx, -dhdy, bumpHeight));

    // do a simple world-space light using your actual light/eye
    vec3 L = normalize(ubo.lightPos - vWorldPos);
    vec3 V = normalize(ubo.eyePos  - vWorldPos);
    vec3 H = normalize(L + V);

    // we need the bump normal in the same space as L and V.
    // the lab’s simple version just uses bumpN directly:
    float diff = max(dot(bumpN, L), 0.0);
    float spec = pow(max(dot(bumpN, H), 0.0), 32.0);

    vec3 ambient  = 0.15 * albedo;
    vec3 diffuse  = diff * albedo;
    vec3 specular = spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, 1.0);
}

