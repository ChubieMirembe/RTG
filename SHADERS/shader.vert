#version 450
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelOverride;
    uint useOverride;
    uint unlit;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec3 vDir;

void main() {
    // Use override transform if present (usually scale for skybox)
    mat4 M = (pc.useOverride != 0u) ? pc.modelOverride : ubo.model;

    // Remove translation from view matrix so the skybox stays centered
    mat4 V = ubo.view;
    V[3] = vec4(0.0, 0.0, 0.0, 1.0);

    // World-space direction for sampling
    vec4 world = M * vec4(inPos, 1.0);
    vDir = (mat3(inverse(V)) * world.xyz);

    // Project and push to the far plane so it surrounds everything
    vec4 clip = ubo.proj * V * vec4(world.xyz, 1.0);
    gl_Position = vec4(clip.xy, clip.w, clip.w);
}
