#version 450

layout(location = 0) in vec3 inParticlePos;
layout(location = 1) in vec2 inCorner; 

layout(location = 0) out vec2 texCoord; // for radial/vertical shaping
layout(location = 1) out float t;       // lifetime / height parameter

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 eyePos;
    float time;
    vec3 _pad;
} ubo;

#define particleSpeed         0.48      
#define particleSpread        0.40    
#define particleSize          0.30    
#define particleSystemHeight  2.0     

void main() {
    float id = inParticlePos.z;
    t = fract(id + particleSpeed * ubo.time);

    vec3 base = vec3(inParticlePos.x, 0.5, inParticlePos.y);

    float angle = 50.0 * id;            
    float radius = particleSpread * (1.0 - t);       

    vec3 center;
    center.x = base.x + radius * cos(angle);
    center.z = base.z + radius * sin(angle);

    center.y = base.y + t * particleSystemHeight;

    mat4 viewInv = inverse(ubo.view);
    vec3 right = viewInv[0].xyz;
    vec3 up    = viewInv[1].xyz;

    float sizeH = particleSize;        // horizontal half-size
    float sizeV = particleSize * 1.8;  // vertical half-size

    vec3 worldPos =
        center +
        right * (inCorner.x * sizeH) +
        up    * (inCorner.y * sizeV);

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);

    texCoord = inCorner * 0.5 + 0.5;
}
