#version 450

// ----------------------------------------------------------
// Inputs from vertex shader
// ----------------------------------------------------------
layout(location = 0) in vec2 texCoord;
layout(location = 1) in float t;  // lifetime / height parameter (0 = bottom, 1 = top)

// ----------------------------------------------------------
// Output
// ----------------------------------------------------------
layout(location = 0) out vec4 outColor;

// Same UBO layout (time is there if you want flicker)
layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 eyePos;
    float time;
    vec3 _pad;
} ubo;

// Simple hash to add a little flicker noise
float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

void main() {
    // ------------------------------------------------------
    // 1) Radial shape: circular sprite in [0,1] space
    // ------------------------------------------------------
    vec2 uv = texCoord * 2.0 - 1.0;  // to [-1,1]
    float r = length(uv);

    // Soft circle mask: 1 in centre, 0 at radius 1
    float radial = smoothstep(1.0, 0.0, r);

    // ------------------------------------------------------
    // 2) Height-based fade (lecture style)
    //    t = 0 ⇒ bottom; t = 1 ⇒ top
    //    bottom should be bright, top fades out
    // ------------------------------------------------------
    float h = clamp(t, 0.0, 1.0);

    // Fire body: strong at bottom, fades to top
    float body = 1.0 - h;  // 1 at bottom, 0 at top

    // Keep a minimum so it never fully disappears (prevents hard popping)
    body = max(body, 0.15);

    // Combine radial falloff with vertical body
    float alpha = radial * body;

    // ------------------------------------------------------
    // 3) Add a little animated flicker so it feels alive
    // ------------------------------------------------------
    float noise = hash21(texCoord * 12.3 + ubo.time * 3.7);
    float flicker = 0.8 + 0.2 * noise;  // 0.8..1.0
    alpha *= flicker;

    // Kill only the very edge
    if (alpha < 0.02)
        discard;

    // ------------------------------------------------------
    // 4) Fire colour ramp: hot at bottom, smoky at top
    // ------------------------------------------------------
    // Use height and radial to shape colour
    float heat = (1.0 - h) * radial;   // hot core at bottom centre

    vec3 colBottom = vec3(1.0, 0.55, 0.10); // bright orange
    vec3 colMid    = vec3(1.0, 0.85, 0.40); // yellowish
    vec3 colTop    = vec3(0.25, 0.20, 0.20); // dark smoke

    vec3 color;
    if (heat > 0.6) {
        // core: orange → yellow
        float k = (heat - 0.6) / 0.4;
        color = mix(colBottom, colMid, k);
    } else if (heat > 0.3) {
        // middle: red/orange-ish
        float k = (heat - 0.3) / 0.3;
        color = mix(vec3(0.6, 0.15, 0.02), colBottom, k);
    } else {
        // top / edges: towards smoke
        float k = heat / 0.3;
        color = mix(colTop, vec3(0.5, 0.2, 0.05), k);
    }

    // Make brighter where alpha is strong
    color *= (0.6 + 0.4 * alpha);

    outColor = vec4(color, alpha);
}
