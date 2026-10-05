#version 330 core

out vec2 uv;

void main() {
    vec2 vertex = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = vertex;
    gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);
}
