#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightColor      = vec3(1.0);
    vec3 ambientMaterial = vec3(0.2, 0.1, 0.2);
    vec3 diffMaterial    = vec3(1.0);
    vec3 specMaterial    = vec3(1.0);

    float shininess = 32.0;

    vec3 N = normalize(fragWorldNormal);
    vec3 L = normalize(ubo.lightPos - fragWorldPos);
    vec3 V = normalize(ubo.eyePos - fragWorldPos);
    vec3 R = reflect(-L, N);

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;

    float spec = pow(max(dot(R, V), 0.0), shininess);
    vec3 specular = specMaterial * lightColor * spec;

    vec3 color = ambientMaterial * lightColor
               + diffMaterial * diffuse
               + specular;

    outColor = vec4(color, 1.0);
}