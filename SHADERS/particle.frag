#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float t;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = texCoord * 2.0 - 1.0;
    float r = length(uv);
    float alphaShape = smoothstep(1.0, 0.0, r);

    float alphaLife = 1.0 - t;
    float alpha = alphaShape * alphaLife;

    if (alpha < 0.01)
        discard;

    vec3 base = mix(vec3(1.0, 0.5, 0.0), vec3(1.0, 1.0, 0.0), (1.0 - t));
    outColor = vec4(base, alpha);
}
