#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float lifeT;

layout(location = 0) out vec4 outColor;

void main() {
    // Turn texCoord into -1..1 space
    vec2 uv = texCoord * 2.0 - 1.0;
    float r = length(uv);

    // Soft circular shape
    float alphaShape = smoothstep(1.0, 0.0, r);

    // Fade-out over lifetime: full at birth, zero at end
    float alphaLife = 1.0 - lifeT;

    float alpha = alphaShape * alphaLife;

    if (alpha < 0.01)
        discard;

    // Smoke colour: bright near start, dark near end
    vec3 startCol = vec3(1.0, 0.7, 0.3);   // near fire
    vec3 endCol   = vec3(0.2, 0.2, 0.2);   // dark smoke
    vec3 color = mix(startCol, endCol, lifeT);

    outColor = vec4(color, alpha);
}
