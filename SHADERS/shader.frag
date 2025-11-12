#version 450
layout(set = 0, binding = 1) uniform samplerCube uSky;

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 outColor;

void main() {
    // Sample cubemap using normalized direction
    vec3 dir = normalize(vDir);
    vec4 color = texture(uSky, dir);

    // Simple gamma correction
    outColor = vec4(pow(color.rgb, vec3(1.0 / 2.2)), 1.0);
}
