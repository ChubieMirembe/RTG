#version 450

layout(location = 0) out vec2 uv;

vec2 POS[3] = vec2[](
    vec2(-1, -1),
    vec2( 3, -1),
    vec2(-1,  3)
);

vec2 UV[3] = vec2[](
    vec2(0, 0),
    vec2(2, 0),
    vec2(0, 2)
);

void main() {
    gl_Position = vec4(POS[gl_VertexIndex], 0, 1);
    uv = UV[gl_VertexIndex];
}
