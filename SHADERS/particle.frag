#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float t;
layout(location = 2) in float fade;

layout(location = 0) out vec4 outColor;

void main() {
    // Round-ish soft particle
    vec2 uv = texCoord * 2.0 - 1.0;
    float r = length(uv);

    float alphaShape = smoothstep(1.0, 0.0, r);

    float alphaLife = fade;
    float alpha = alphaShape * alphaLife;

    if (alpha < 0.02)
        discard;

    // Warm smoke colour: bright at base, darker as it rises
    vec3 hot   = vec3(1.0, 0.6, 0.1);
    vec3 smoke = vec3(0.3, 0.3, 0.3);
    vec3 col   = mix(hot, smoke, t);

    outColor = vec4(col, alpha);
}
