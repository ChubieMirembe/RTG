#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec3 vColor;
layout(location = 3) out vec2 vUV;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 N = mat3(transpose(inverse(ubo.model)));
    vWorldNormal = normalize(N * inNormal);

    vColor = inColor;
    vUV = inUV * vec2(2.0,2.0);  // pass to fragment shader

    gl_Position = ubo.proj * ubo.view * worldPos;
}
