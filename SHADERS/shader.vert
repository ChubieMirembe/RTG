#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 viewPos;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec3 vColor;
layout(location = 3) out vec2 vUV;

// now these are *directions* in tangent space
layout(location = 4) out vec3 fragLightDir_tangent;
layout(location = 5) out vec3 fragViewDir_tangent;
layout(location = 6) out vec3 fragPos_tangent;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 Nmat = mat3(transpose(inverse(ubo.model)));
    vWorldNormal = normalize(Nmat * inNormal);

    vColor = inColor;
    vUV = inUV * vec2(2.0, 2.0);

    gl_Position = ubo.proj * ubo.view * worldPos;

    // ----- Exercise 2 bit (fixed)
    vec3 T = normalize(Nmat * inTangent);
    vec3 B = normalize(Nmat * inBinormal);
    vec3 N = normalize(Nmat * inNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    // make DIRECTIONS in world space
    vec3 fragPos_world   = vWorldPos;
    vec3 lightDir_world  = ubo.lightPos - fragPos_world;
    vec3 viewDir_world   = ubo.viewPos  - fragPos_world;

    // send to fragment as tangent-space directions
    fragLightDir_tangent = TBN * lightDir_world;
    fragViewDir_tangent  = TBN * viewDir_world;
    // sometimes the fragment wants the pos too (for parallax)
    fragPos_tangent      = TBN * fragPos_world;
}
