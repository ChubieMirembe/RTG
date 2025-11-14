#version 450

layout(location = 0) in vec3 inParticlePos; // x,y = emitter on cube top, z = seed
layout(location = 1) in vec2 inCorner;      // (-1,-1..1,1)

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float t;           // life 0..1
layout(location = 2) out float fade;        // extra fade for smoke

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
    float time;
} ubo;

// Tunables
const float RISE_SPEED     = 1.2;   // vertical speed
const float LIFE_TIME      = 2.5;   // seconds until particle dies
const float SPREAD_XZ      = 0.6;   // side drift strength
const float SIZE_START     = 0.25;  // start quad half-size in world units
const float SIZE_END       = 0.6;   // end size
const float EMISSION_JITTER = 0.7;  // offset emission times

void main() {
    float seed = inParticlePos.z;

    // Each particle loops on [0,1)
    float timeOffset = seed * EMISSION_JITTER * LIFE_TIME;
    float life = mod(ubo.time + timeOffset, LIFE_TIME) / LIFE_TIME;
    t = life;

    // Upward motion
    float y = inParticlePos.y + RISE_SPEED * life;

    // Curly sideways drift based on seed
    float ang  = seed * 23.0 + life * 6.28318;
    float xOff = SPREAD_XZ * life * 0.5 * cos(ang);
    float zOff = SPREAD_XZ * life * 0.5 * sin(ang);

    vec3 baseWorld = vec3(inParticlePos.x + xOff, y, 0.0 + zOff);

    // Camera-facing billboard
    mat4 viewInv = inverse(ubo.view);
    vec3 right = normalize(viewInv[0].xyz);
    vec3 up    = normalize(viewInv[1].xyz);

    // Size grows over life
    float size = mix(SIZE_START, SIZE_END, life);

    vec3 worldPos = baseWorld + size * (inCorner.x * right + inCorner.y * up);

    // Smoke fades out near end of life
    fade = 1.0 - smoothstep(0.7, 1.0, life);

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
    texCoord = inCorner * 0.5 + 0.5;
}
