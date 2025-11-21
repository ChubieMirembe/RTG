#version 450

layout(location = 0) in vec2 uv;
layout(set = 0, binding = 1) uniform sampler2D sceneTexture;
layout(location = 0) out vec4 outColor;

void main() {
    // Controls how strong the blur is
    int boxSize = 3;          // increase for more blur (1–10)
    float stepSize = 2.5;     // increase for more spread

    vec2 texel = stepSize / vec2(textureSize(sceneTexture, 0));
    vec3 sum = vec3(0.0);

    int total = 0;

    for (int x = -boxSize; x <= boxSize; x++) {
        for (int y = -boxSize; y <= boxSize; y++) {
            sum += texture(sceneTexture, uv + vec2(x, y) * texel).rgb;
            total++;
        }
    }

    outColor = vec4(sum / float(total), 1.0);
}
