#version 450

// inputs: must match your C++ vertex attributes
// UBO (set=0, binding=0) must match C++
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

// Match your C++ locations: pos=0, color=1, normal=2, texCoord=3
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec3 vColor;        // (optional if you still want color)
layout(location = 3) out vec2 vUV;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;

    // proper normal transform
    mat3 N = mat3(transpose(inverse(ubo.model)));
    vWorldNormal = normalize(N * inNormal);

    vColor = inColor;
    vUV = inTexCoord;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
