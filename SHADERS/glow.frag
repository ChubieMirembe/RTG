#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

// RTT image from pass 1
layout(set = 0, binding = 1) uniform sampler2D sceneTex;

// Time + intensity from C++
layout(push_constant) uniform FireParams {
    float time;
    float intensity;
    float pad0;
    float pad1;
} fire;

void main() {
    vec2 texSize = vec2(textureSize(sceneTex, 0));
    vec2 texel   = 1.5 / texSize; // base blur step

    // original sharp cube color
    vec4 sharp = texture(sceneTex, uv);

    // heat distortion
    float noise = sin(uv.y * 60.0 + fire.time * 10.0) *
                  cos(uv.x * 40.0 - fire.time * 7.0);
    vec2 wobble = texel * 5.0 * noise;

    // radial blur around cube
    int   radius    = 3;
    vec3  blurSum   = vec3(0.0);
    float weightSum = 0.0;

    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2  o     = vec2(x, y);
            float dist  = length(o);
            float w     = max(0.0, 1.0 - dist / float(radius + 1));

            vec2 sampleUV = uv + wobble + o * texel;
            vec3 sampleCol = texture(sceneTex, sampleUV).rgb;

            blurSum   += sampleCol * w;
            weightSum += w;
        }
    }

    vec3 blurred = blurSum / max(weightSum, 0.0001);

    // aura = bright edges
    vec3 aura = max(blurred - sharp.rgb, 0.0);

    // fire tint and flicker
    vec3 fireTint = vec3(1.0, 0.6, 0.1);
    float flicker = 0.7 + 0.3 * sin(fire.time * 8.0 + uv.y * 50.0);

    vec3 color = sharp.rgb + aura * fire.intensity * fireTint * flicker;

    outColor = vec4(color, 1.0);
}
