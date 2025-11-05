#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vColor;
layout(location = 3) in vec2 vUV;

layout(location = 4) in vec3 fragLightDir_tangent;
layout(location = 5) in vec3 fragViewDir_tangent;
layout(location = 6) in vec3 fragPos_tangent;

layout(location = 0) out vec4 outColor;

void main() {
    // base colour
    vec3 albedo = texture(colSampler, vUV).rgb;

    // normal from normal map
    vec3 N_tangent = texture(normalSampler, vUV).rgb;
    N_tangent = normalize(N_tangent * 2.0 - 1.0);

    // exaggerate bump
    N_tangent.z *= 0.8;  
    N_tangent = normalize(N_tangent);

    // directions in tangent space
    vec3 L = normalize(fragLightDir_tangent);
    vec3 V = normalize(fragViewDir_tangent);
    vec3 H = normalize(L + V);

    float diff = max(dot(N_tangent, L), 0.0);
    float spec = pow(max(dot(N_tangent, H), 0.0), 32.0);

    vec3 ambient  = 0.15 * albedo;
    vec3 diffuse  = diff * albedo;
    vec3 specular = spec * vec3(1.0);

    vec3 color = ambient + diffuse + specular;
    outColor = vec4(color, 1.0);

}
