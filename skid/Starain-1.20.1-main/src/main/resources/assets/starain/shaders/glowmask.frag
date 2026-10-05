#version 330 core

precision mediump float;

in vec2 uv;
out vec4 color;

const int MAX_SLOTS = 128;

uniform vec3 uRects[MAX_SLOTS];
uniform int uRectCount;
uniform vec2 uScreen;

void main() {
    vec2 p = vec2(uv.x, 1.0 - uv.y) * uScreen;
    float a = 0.0;
    for (int i = 0; i < uRectCount; i++) {
        vec3 head = uRects[i * 2];
        vec3 tail = uRects[i * 2 + 1];
        vec2 size = vec2(head.z, tail.x);
        float rad = tail.y;
        vec2 c = head.xy + size * 0.5;
        vec2 h = max(size * 0.5 - rad, vec2(0.0));
        vec2 d = abs(p - c) - h;
        float dist = length(max(d, vec2(0.0))) + min(max(d.x, d.y), 0.0) - rad;
        a = max(a, (1.0 - smoothstep(-0.5, 0.5, dist)) * tail.z);
    }
    if (a <= 0.002) discard;
    color = vec4(1.0, 1.0, 1.0, a);
}
