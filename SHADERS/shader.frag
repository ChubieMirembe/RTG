#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vColor;    // optional
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(vWorldNormal);
    vec3 L = normalize(ubo.lightPos - vWorldPos);
    vec3 V = normalize(ubo.eyePos   - vWorldPos);
    vec3 R = reflect(-L, N);

    // simple Phong
    vec3 tex = texture(texSampler, vUV).rgb;

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(R, V), 0.0), 32.0);

    vec3 ambient  = 0.15 * tex;
    vec3 diffuse  = 0.85 * diff * tex;
    vec3 specular = 0.15 * spec * vec3(1.0);

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
