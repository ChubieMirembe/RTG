#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float t;  // lifetime / height parameter (0 = bottom, 1 = top)

layout(location = 0) out vec4 outColor;

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 eyePos;
    float time;
    vec3 _pad;
} ubo;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

void main() {
    vec2 uv = texCoord * 2.0 - 1.0;  
    float r = length(uv);

    float radial = smoothstep(1.0, 0.0, r);

    float h = clamp(t, 0.0, 1.0);
    float body = 1.0 - h;  
    body = max(body, 0.15);

    float alpha = radial * body;

    float noise = hash21(texCoord * 12.3 + ubo.time * 3.7);
    float flicker = 0.8 + 0.2 * noise;  // 0.8..1.0
    alpha *= flicker;

    if (alpha < 0.02)
        discard;

    float heat = (1.0 - h) * radial;   // hot core at bottom centre

    vec3 colBottom = vec3(1.0, 0.55, 0.10); // bright orange
    vec3 colMid    = vec3(1.0, 0.85, 0.40); // yellowish
    vec3 colTop    = vec3(0.25, 0.20, 0.20); // dark smoke

    vec3 color;
    if (heat > 0.6) {
        float k = (heat - 0.6) / 0.4;
        color = mix(colBottom, colMid, k);
    } else if (heat > 0.3) {
        float k = (heat - 0.3) / 0.3;
        color = mix(vec3(0.6, 0.15, 0.02), colBottom, k);
    } else {
        float k = heat / 0.3;
        color = mix(colTop, vec3(0.5, 0.2, 0.05), k);
    }

    color *= (0.6 + 0.4 * alpha);

    outColor = vec4(color, alpha);
}
