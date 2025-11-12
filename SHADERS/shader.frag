#version 450
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

layout(set = 0, binding = 1) uniform samplerCube uSky;

layout(location = 0) in vec3 vDir;
layout(location = 1) in vec3 vWorldPos;
layout(location = 2) in vec3 vWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    if (pc.unlit != 0u) {
        vec3 dir = normalize(vDir);
        vec3 col = texture(uSky, dir).rgb;
        outColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
        return;
    }

    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(ubo.eyePos.xyz - vWorldPos);
    vec3 R = reflect(-V, N);
    vec3 col = texture(uSky, R).rgb;

    float F0 = 0.04;
    float F = F0 + (1.0 - F0) * pow(clamp(1.0 - dot(N, V), 0.0, 1.0), 5.0);
    col = mix(col * 0.8, col, F);

    outColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
}
