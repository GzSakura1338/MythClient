#version 330 core

uniform vec2 u_Size;

out vec2 v_TexCoord;
out vec2 v_OneTexel;

// Mio's base.vert drew a quad from a vertex buffer; this is the same thing from gl_VertexID,
// matching gaussian.vsh so the pass needs no attributes at all.
void main() {
    vec2 vertex = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_TexCoord = vertex;
    v_OneTexel = 1.0 / u_Size;
    gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);
}
