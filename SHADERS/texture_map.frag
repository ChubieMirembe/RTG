#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D sceneTexture;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 sceneSample = texture(sceneTexture, fragTexCoord);
    outColor = sceneSample;
}
