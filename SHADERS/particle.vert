#version 450

layout(location = 0) in vec3 inParticlePos; // x,y ignored, z = particle id
layout(location = 1) in vec2 inCorner;      // quad corner (-1,-1..1,1)

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float lifeT;

// Same UBO layout as your C++ side
layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 eyePos;
    float time;
    vec3 _pad;  // matches C++ float[3] padding
} ubo;

// Tunable smoke behaviour
#define EMISSION_RATE   0.45      // how fast new particles appear
#define RISE_HEIGHT     4.0       // how high the smoke rises
#define SPREAD_RADIUS   0.6       // sideways spread
#define QUAD_SIZE       0.35      // size of each sprite

// Simple hash to get repeatable pseudo-random per-id
float hash11(float x) {
    return fract(sin(x * 12.9898) * 43758.5453123);
}

void main() {
    // Treat z as particle "id"
    float id = inParticlePos.z;

    // Normalized lifetime 0..1 that loops over time
    lifeT = fract(ubo.time * EMISSION_RATE + id * 0.37);

    // Base position: top of the cube at origin
    vec3 base = vec3(0.0, 0.5, 0.0);

    // Get two random values from id
    float r1 = hash11(id * 17.0);
    float r2 = hash11(id * 53.0);

    // Angle around the vertical axis
    float angle = r1 * 6.2831853; // 2π

    // Radius shrinks as the particle ages so it converges to the plume center
    float radius = SPREAD_RADIUS * (1.0 - lifeT);

    // Horizontal offset (swirl around Y axis)
    vec3 horizontal = vec3(
        radius * cos(angle),
        0.0,
        radius * sin(angle)
    );

    // Vertical rise with lifetime
    float height = lifeT * RISE_HEIGHT;

    vec3 centerWorld = base + horizontal + vec3(0.0, height, 0.0);

    // Camera-facing quad (billboard) using view inverse
    mat4 viewInv = inverse(ubo.view);
    vec3 right = viewInv[0].xyz;
    vec3 up    = viewInv[1].xyz;

    vec3 worldPos = centerWorld + QUAD_SIZE * (inCorner.x * right + inCorner.y * up);

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);

    texCoord = inCorner * 0.5 + 0.5;
}
