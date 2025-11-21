#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D sceneTexture;

void main() {
    int   boxSize   = 7;   
    float stepSize  = 6.0; 

    vec2 texel = stepSize / vec2(textureSize(sceneTexture, 0));

    vec3 sum   = vec3(0.0);
    int  total = 0;

    for (int x = -boxSize; x <= boxSize; x++) {
        for (int y = -boxSize; y <= boxSize; y++) {
            vec2 offset = vec2(x, y) * texel;
            sum += texture(sceneTexture, uv + offset).rgb;
            total++;
        }
    }

    vec3 blurred = sum / float(total);
    vec3 sharp = texture(sceneTexture, uv).rgb;

    float glowStrength = 2.5; 
    vec3 result = sharp + blurred * glowStrength;

    outColor = vec4(result, 1.0);
}
