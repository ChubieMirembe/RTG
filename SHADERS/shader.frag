#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D heightSampler;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vColor;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(colSampler, vUV).rgb;
    float h = texture(heightSampler, vUV).r;

    float dhdx = dFdx(h);
    float dhdy = dFdy(h);

    float bumpStrength = 50.0;
    vec3 bumpN = normalize(vec3(-dhdx * bumpStrength, -dhdy * bumpStrength, 1.0));

    vec3 L = normalize(ubo.lightPos - vWorldPos);
    vec3 V = normalize(ubo.eyePos  - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(bumpN, L), 0.0);
    float spec = pow(max(dot(bumpN, H), 0.0), 32.0);

    vec3 ambient  = 0.15 * albedo;
    vec3 diffuse  = diff * albedo;
    vec3 specular = spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, 1.0);
}

