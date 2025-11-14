#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float t;

layout(location = 0) out vec4 outColor;

void main() {
    // Soft round sprite
    vec2 uv = texCoord * 2.0 - 1.0;
    float r = length(uv);
    float alphaShape = smoothstep(1.0, 0.0, r);

    // Fade in at the start, fade out at the end
    float fadeIn  = smoothstep(0.0, 0.2, t);
    float fadeOut = 1.0 - t;
    float alphaLife = fadeIn * fadeOut;

    float alpha = alphaShape * alphaLife;
    if (alpha < 0.01)
        discard;

    // Grey smoke: dark at base, lighter as it rises
    vec3 smokeColor = mix(vec3(0.05, 0.05, 0.05),
                          vec3(0.8, 0.8, 0.8),
                          t);

    outColor = vec4(smokeColor, alpha);
}
