#version 450

layout(location = 0) in vec3 inParticlePos; // x,z = base offset on emitter, z = seed
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

// Tunables for the smoke motion
const float particleSpeed = 0.4;
const float emitterHeight = 0.5;   // top face of cube (cube is [-0.5,0.5])
const float maxRise       = 4.0;   // how high the smoke goes
const float baseRadius    = 0.35;  // how wide the column gets
const float particleSize  = 0.25;  // quad size

void main() {
    // Seed for this particle
    float seed = inParticlePos.z;

    // Normalised life time in [0,1], looping over time
    t = fract(ubo.time * particleSpeed + seed);

    // Vertical motion
    float rise = maxRise * t;

    // A bit of swirling as it goes up
    float swirlPhase  = seed * 6.2831853 + t * 3.0;
    float swirlRadius = baseRadius * t;

    // Emitter is the centre of the top face of the cube
    vec3 emitter = vec3(0.0, emitterHeight, 0.0);

    vec3 pos;
    // Use inParticlePos.x as local x offset, inParticlePos.y as local z offset
    pos.x = emitter.x + inParticlePos.x + cos(swirlPhase) * swirlRadius;
    pos.z = emitter.z + inParticlePos.y + sin(swirlPhase) * swirlRadius;
    pos.y = emitter.y + rise;

    // Camera-facing quad (billboard)
    mat4 viewInv = inverse(ubo.view);
    vec3 right = viewInv[0].xyz;
    vec3 up    = viewInv[1].xyz;

    vec3 worldPos = pos + particleSize * (inCorner.x * right + inCorner.y * up);

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
    texCoord = inCorner * 0.5 + 0.5;
}
