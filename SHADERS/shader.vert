#version 450

// Build VIEW and PROJECTION directly in the shader from camera params
layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;      // per-object model (still built on CPU)
    vec4 eye;        // xyz = camera position
    vec4 center;     // xyz = look-at target
    vec4 up;         // xyz = world up (e.g., 0,1,0)
    vec4 cam;        // x=fovy (radians), y=aspect, z=zNear, w=zFar
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    // --- View matrix (manual lookAt)
    vec3 f = normalize(ubo.center.xyz - ubo.eye.xyz);   // forward
    vec3 s = normalize(cross(f, ubo.up.xyz));           // right
    vec3 u = cross(s, f);                               // true up

    mat4 view = mat4(1.0);
    view[0][0] =  s.x; view[1][0] =  s.y; view[2][0] =  s.z; view[3][0] = -dot(s, ubo.eye.xyz);
    view[0][1] =  u.x; view[1][1] =  u.y; view[2][1] =  u.z; view[3][1] = -dot(u, ubo.eye.xyz);
    view[0][2] = -f.x; view[1][2] = -f.y; view[2][2] = -f.z; view[3][2] =  dot(f, ubo.eye.xyz);
    view[0][3] =  0.0; view[1][3] =  0.0; view[2][3] =  0.0; view[3][3] =  1.0;

    // --- Projection matrix (manual perspective, Vulkan-convention)
    float fovy   = ubo.cam.x;
    float aspect = ubo.cam.y;
    float zNear  = ubo.cam.z;
    float zFar   = ubo.cam.w;

    float fct = 1.0 / tan(fovy * 0.5);

    mat4 proj = mat4(0.0);
    proj[0][0] = fct / aspect;
    proj[1][1] = fct;
    proj[2][2] = (zFar + zNear) / (zNear - zFar);
    proj[2][3] = -1.0;
    proj[3][2] = (2.0 * zFar * zNear) / (zNear - zFar);

    // Vulkan clip-space Y flip
    proj[1][1] *= -1.0;

    gl_Position = proj * view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
