#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

// color / diffuse texture
layout(set = 0, binding = 1) uniform sampler2D colorTex;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec4 outColor;

const float BumpDensity = 6.0;
const float Radius      = 0.6;

vec3 bumpNormal(vec2 xy)
{
    // start flat
    vec3 N = vec3(0.0, 0.0, 1.0);

    // tile to [-1,1]
    vec2 st = 2.0 * fract(xy) - 1.0;

    float r2 = Radius * Radius;
    float d2 = dot(st, st);
    float R2 = r2 - d2;

    if (R2 > 0.0) {
        float z = sqrt(R2);
        N.xy = st / z;
    }
    return normalize(N);
}

void main()
{
    // 1) procedural normal in world space (projected)
    //    using world XY like you had
    vec2 p = vWorldPos.xy * BumpDensity;
    vec3 N = bumpNormal(p);

    // 2) lighting vectors in world space
    vec3 L = normalize(ubo.lightPos - vWorldPos);
    vec3 V = normalize(ubo.eyePos  - vWorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);

    // 3) sample texture for base colour
    vec3 albedo = texture(colorTex, vUV).rgb;

    // 4) Blinn–Phong with your procedural normal
    vec3 ambient  = 0.15 * albedo;
    vec3 diffuse  = NdotL * albedo;
    float spec    = pow(max(dot(N, H), 0.0), 32.0);
    vec3 specular = vec3(1.0) * spec * 0.5;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
