#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;   // unused in lighting, kept for compatibility
layout(location = 2) in vec3 inNormal;

layout(binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    vec3 eyePos; float _pad0;
    vec3 light1Pos; float _pad1;
    vec3 light1Col; float _pad2;
    vec3 light2Pos; float _pad3;
    vec3 light2Col; float _pad4;
} ubo;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 Ka;
    vec4 Kd;
    vec4 Ks; // Ks.w = shininess
} pc;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;

    // Transform normal to world space (assumes uniform scale; for non-uniform, use inverse transpose)
    mat3 normalMat = mat3(pc.model);
    vWorldNormal = normalize(normalMat * inNormal);

    gl_Position = ubo.proj * ubo.view * worldPos;
}
