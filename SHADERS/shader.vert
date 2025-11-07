#version 450

// vertex inputs from your C++ Vertex struct
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;    // not strictly needed but we can pass it
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal; // you called it binormal in C++

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelOverride;
    uint useOverride;
    uint unlit;
    uint _pad0;
    uint _pad1;
} pc;

// what we pass to fragment
layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vT;
layout(location = 3) out vec3 vB;
layout(location = 4) out vec3 vN;
layout(location = 5) out vec3 vColor;

void main()
{
    // choose model matrix (override for your light-sphere gizmo, etc.)
    mat4 M = (pc.useOverride == 1u) ? pc.modelOverride : ubo.model;

    vec4 worldPos = M * vec4(inPos, 1.0);

    vWorldPos = worldPos.xyz;
    vUV       = inUV;

    // transform TBN to world space
    // note: if your model matrix has non-uniform scale, you'd want to use the
    // inverse-transpose 3x3. For now, this should be okay for unit cubes.
    mat3 M3 = mat3(M);
    vT = normalize(M3 * inTangent);
    vB = normalize(M3 * inBinormal);
    vN = normalize(M3 * inNormal);

    vColor = inColor;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
