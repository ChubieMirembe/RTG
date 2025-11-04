#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

// Two textures: coin (binding 1) and tile (binding 2)
layout(set = 0, binding = 1) uniform sampler2D texSampler1;
layout(set = 0, binding = 2) uniform sampler2D texSampler2;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vColor;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample both textures
    vec3 color1 = texture(texSampler1, vUV).rgb;          // coin texture
    vec3 color2 = texture(texSampler2, vUV * 2.0).rgb;    // tile texture (scaled UVs)

    // Simple diffuse lighting
    vec3 N = normalize(vWorldNormal);
    vec3 L = normalize(ubo.lightPos - vWorldPos);
    float NdotL = max(dot(N, L), 0.0);

    // Blend both textures evenly (50% each)
    vec3 mixedColor = mix(color1, color2, 0.3);

    // Ambient and diffuse contributions
    vec3 ambient = mixedColor * 0.15;
    vec3 diffuse = mixedColor * NdotL;

    vec3 finalColor = ambient + diffuse;
    outColor = vec4(finalColor, 1.0);
}
