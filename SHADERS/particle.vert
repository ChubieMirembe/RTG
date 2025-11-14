#version 450

layout(location = 0) in vec3 inParticlePos; // x,y = center, z = id/seed
layout(location = 1) in vec2 inCorner;      // (-1,-1..1,1)

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float t;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
    float time;
} ubo;

#define particleSpeed 0.48
#define particleSpread 20.48
#define particleSize 6.0

void main() {
    float id = inParticlePos.z;
    t = fract(id + particleSpeed * ubo.time);

    vec3 pos;
    pos.x = particleSpread * t * cos(50.0 * id);
    pos.y = particleSpread * t * 0.02;
    pos.z = particleSpread * t * sin(120.0 * id);

    mat4 viewInv = inverse(ubo.view);
    vec3 right = viewInv[0].xyz;
    vec3 up    = viewInv[1].xyz;

    vec3 worldPos = pos + particleSize * (inCorner.x * right + inCorner.y * up);

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
    texCoord = inCorner * 0.5 + 0.5;
}
