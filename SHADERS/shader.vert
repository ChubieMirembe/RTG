#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 eyePos;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelOverride;
    uint useOverride;
    uint unlit;
    uint _pad0;
    uint _pad1;
} pc;

layout(location = 0) out vec3 vDir;
layout(location = 1) out vec3 vWorldPos;
layout(location = 2) out vec3 vWorldNormal;

void main() {
    mat4 M = (pc.useOverride != 0u) ? pc.modelOverride : ubo.model;

    vec4 world = M * vec4(inPos, 1.0);
    vWorldPos = world.xyz;
    vWorldNormal = normalize(mat3(M) * inNormal);

    mat4 V = ubo.view;
    if (pc.unlit != 0u) {
        // Skybox: no translation, depth at far plane
        mat4 Vsky = V;
        Vsky[3] = vec4(0.0, 0.0, 0.0, 1.0);
        vDir = mat3(inverse(Vsky)) * world.xyz;
        vec4 clip = ubo.proj * Vsky * vec4(world.xyz, 1.0);
        gl_Position = vec4(clip.xy, clip.w, clip.w);
    } else {
        // Meshes: full view matrix, normal depth
        vDir = mat3(inverse(V)) * world.xyz;
        gl_Position = ubo.proj * V * vec4(world.xyz, 1.0);
    }
}
