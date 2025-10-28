#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos; // kept for later work
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;   // not used for lighting here
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragWorldNormal;

void main() {
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPosition.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    fragWorldNormal = normalize(normalMatrix * inNormal);

    gl_Position = ubo.proj * ubo.view * worldPosition;
}
