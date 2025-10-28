#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec3 inNormal;

layout(location=0) out vec3 fragColor;

void main() {
    // Transform to world space
    vec3 worldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
    vec3 worldNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
    vec3 norm = normalize(worldNormal);

    vec3 lightColor = vec3(1.0);
    vec3 ambientMaterial = vec3(0.2, 0.1, 0.2);
    vec3 diffMaterial = vec3(1.0);

    // Diffuse
    vec3 lightDir = normalize(ubo.lightPos - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    vec3 viewDir = normalize(ubo.eyePos - worldPos);
    vec3 reflectDir = normalize(reflect(-lightDir, norm));
    float shininess = 32.0;
    float spec = pow(max(dot(reflectDir, viewDir), 0.0), shininess);
    vec3 specMaterial = vec3(1.0);
    vec3 specular = specMaterial * lightColor * spec;

    // Output lighting result (per-vertex)
    fragColor = ambientMaterial * lightColor + diffMaterial * diffuse + specular;

    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
}
