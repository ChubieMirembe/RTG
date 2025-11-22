#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D sceneTex;

layout(push_constant) uniform FireParams {
    float time;
    float intensity;
    float pad0;
    float pad1;
} fire;

// small hash for per-tap variation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    vec2 texSize = vec2(textureSize(sceneTex, 0));

    // OPTION A: base blur step
    vec2 texel = 3.5 / texSize;

    // original sharp cube color
    vec4 sharp = texture(sceneTex, uv);

    // global heat wobble
    float noise = sin(uv.y * 60.0 + fire.time * 10.0) *
                  cos(uv.x * 40.0 - fire.time * 7.0);
    vec2 wobble = texel * 8.0 * noise;

    // base blur radius
    int   radius    = 10;
    vec3  blurSum   = vec3(0.0);
    float weightSum = 0.0;

    // remap v so 0 is bottom, 1 is top
    float v = uv.y;

    // approximate cube centre horizontally (middle of screen)
    float centerX = 0.5;
    float distFromCenter = abs(uv.x - centerX);

    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 o = vec2(x, y);

            // [NARROW-X] elliptical distance: shrink horizontal reach
            // 0.35 makes flames about 1/3 as wide vs tall
            float dist = length(vec2(o.x * 0.35, o.y));

            float w = max(0.0, 1.0 - dist / float(radius + 1));

            float nTap = hash(o * 3.17 + uv * 25.0 + fire.time);

            // upward base factor
            float baseUp = float(radius + 1 - y) / float(radius + 1);
            baseUp = max(baseUp, 0.0);

            // columns vary in height
            float uneven = 0.6 + 0.8 * nTap;

            // fade flames as they rise
            float heightMask = 1.0 - smoothstep(0.45, 0.85, v);

            // [NARROW-X] fade flames away from cube centre horizontally
            //  - 0 distance = full strength
            //  - >0.25 away ~ almost gone
            float sideMask = 1.0 - smoothstep(0.18, 0.25, distFromCenter);

            float upward = baseUp * uneven * heightMask * sideMask;
            upward = pow(upward, 1.3);

            // overall height
            float stretch = 0.05;

            vec2 sampleUV =
                uv
                + wobble
                + o * texel
                + vec2(0.0, upward * stretch);

            vec3 sampleCol = texture(sceneTex, sampleUV).rgb;

            // patchy halo
            float weightJitter = 0.7 + 0.6 * (nTap - 0.5);
            blurSum   += sampleCol * w * weightJitter;
            weightSum += w * weightJitter;
        }
    }

    vec3 blurred = blurSum / max(weightSum, 0.0001);

    vec3 aura = max(blurred - sharp.rgb, 0.0);

    vec3 fireTint = vec3(1.0, 0.6, 0.1);
    float flicker = 0.7 + 0.3 * sin(fire.time * 8.0 + uv.y * 50.0);

    vec3 color = sharp.rgb + aura * fire.intensity * fireTint * flicker;

    outColor = vec4(color, 1.0);
}
