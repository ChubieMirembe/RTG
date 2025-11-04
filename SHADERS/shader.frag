#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

// Two textures: rock (binding 1) and wood (binding 2)
layout(set = 0, binding = 1) uniform sampler2D texSampler1;  // Rock texture
layout(set = 0, binding = 2) uniform sampler2D texSampler2;  // Wood texture

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vColor;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

bool isRearFaceByNormal(vec3 worldNormal) {
    vec3 n = normalize(worldNormal);
    vec3 an = abs(n);

    if (an.x >= an.y && an.x >= an.z) {
        return (n.x < 0.0);
    }
    else if (an.z >= an.x && an.z >= an.y) {
        return (n.z < 0.0);
    }
    return false;
}

void main() {
    // Sample both textures
    vec3 color1 = texture(texSampler1, vUV).rgb;  // Rock texture
    vec3 color2 = texture(texSampler2, vUV).rgb;  // Wood texture

    // Transform the normal to view space
    vec3 normalViewSpace = normalize(mat3(transpose(inverse(ubo.model))) * vWorldNormal);

    // Check if the fragment is a rear face based on normal direction
    bool isRearFace = isRearFaceByNormal(normalViewSpace);

    // Select the texture based on the face orientation
    vec3 finalColor;
    if (isRearFace) {
        // Inside faces: Use wood texture
        finalColor = color2;
    } else {
        // Outside faces: Use rock texture
        finalColor = color1;
    }

    // Simple diffuse lighting
    vec3 N = normalize(vWorldNormal);
    vec3 L = normalize(ubo.lightPos - vWorldPos);
    float NdotL = max(dot(N, L), 0.0);

    // Ambient and diffuse contributions
    vec3 ambient = finalColor * 0.15;
    vec3 diffuse = finalColor * NdotL;

    vec3 mixedColor = ambient + diffuse;
    outColor = vec4(mixedColor, 1.0);
}

