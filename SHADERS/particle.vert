#version 450

// ----------------------------------------------------------
// Inputs
// ----------------------------------------------------------
layout(location = 0) in vec3 inParticlePos; // x,y = emitter base on cube top, z = particle id
layout(location = 1) in vec2 inCorner;      // quad corner (-1,-1 .. 1,1)

// ----------------------------------------------------------
// Outputs to fragment shader
// ----------------------------------------------------------
layout(location = 0) out vec2 texCoord; // for radial/vertical shaping
layout(location = 1) out float t;       // lifetime / height parameter

// ----------------------------------------------------------
// UBO (matches your C++ UniformBufferObject)
// ----------------------------------------------------------
layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 eyePos;
    float time;
    vec3 _pad;
} ubo;

// ----------------------------------------------------------
// Tunable fire parameters (lecture-style)
// ----------------------------------------------------------
#define particleSpeed         0.48      // how fast particles loop in time
#define particleSpread        0.40      // horizontal spread radius near base
#define particleSize          0.30      // base sprite size
#define particleSystemHeight  2.0       // total fire height above cube

void main() {
    // ------------------------------------------------------
    // 1) Time slice: lecture’s looping formula
    //      t = fract(id + particleSpeed * time)
    //    Each particle always exists, just moves from bottom
    //    to top and loops smoothly.
    // ------------------------------------------------------
    float id = inParticlePos.z;
    t = fract(id + particleSpeed * ubo.time);

    // ------------------------------------------------------
    // 2) Base position: top face of the cube
    //    Your cube is -0.5..0.5 in Y, so top is at y = 0.5
    //    Use inParticlePos.x/y for emitter disc on that top.
    // ------------------------------------------------------
    vec3 base = vec3(inParticlePos.x, 0.5, inParticlePos.y);

    // ------------------------------------------------------
    // 3) Horizontal swirl (semi-random using id)
    //    Radius shrinks with t so the plume tightens higher up
    // ------------------------------------------------------
    float angle = 50.0 * id;                         // pseudo-random sway
    float radius = particleSpread * (1.0 - t);       // wide near bottom, tight near top

    vec3 center;
    center.x = base.x + radius * cos(angle);
    center.z = base.z + radius * sin(angle);

    // vertical rise along +Y
    center.y = base.y + t * particleSystemHeight;

    // ------------------------------------------------------
    // 4) Billboarding: align quad with camera view plane
    //    Use inverse(view) to get camera-right and camera-up
    // ------------------------------------------------------
    mat4 viewInv = inverse(ubo.view);
    vec3 right = viewInv[0].xyz;
    vec3 up    = viewInv[1].xyz;

    // Slightly stretch flames vertically
    float sizeH = particleSize;        // horizontal half-size
    float sizeV = particleSize * 1.8;  // vertical half-size

    vec3 worldPos =
        center +
        right * (inCorner.x * sizeH) +
        up    * (inCorner.y * sizeV);

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);

    // Map from [-1,1] → [0,1] for texturing
    texCoord = inCorner * 0.5 + 0.5;
}
