#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    // Per-instance offset along X: ..., -1.5, 1.5, 4.5, ...
    float idx = float(gl_InstanceIndex);
    vec3 offset = vec3(idx * 2.0 - 1.5, 0.0, 0.0);

    // Build translation matrix (column-major)
    mat4 T = mat4(1.0);
    T[3] = vec4(offset, 1.0);

    mat4 instanceModel = ubo.model * T;
    gl_Position = ubo.proj * ubo.view * instanceModel * vec4(inPosition, 1.0);
    fragColor = inColor;
}