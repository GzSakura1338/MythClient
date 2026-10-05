#version 330 core

precision mediump float;

in vec2 uv;
out vec4 color;

const int DIRECTIONS = 16;
const int STEPS = 8;
const int MAX_STOPS = 128;
const float TAU = 6.28318530718;

uniform sampler2D uMask;
uniform vec2 uTexelSize;
uniform vec3 uColors[MAX_STOPS];
uniform int uCount;
uniform float uAlpha;
uniform float uRadius;
uniform float uStrength;
uniform float uInner;
uniform float uVertical;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    float center = texture(uMask, uv).a;
    float jitter = hash(uv);

    float sumOuter = 0.0;
    float sumInner = 0.0;
    float sumWeight = 0.0;

    for (int d = 0; d < DIRECTIONS; d++) {
        float a = TAU * (float(d) + jitter) / float(DIRECTIONS);
        vec2 dir = vec2(cos(a), sin(a)) * uTexelSize * uRadius;
        for (int s = 0; s < STEPS; s++) {
            float t = (float(s) + jitter) / float(STEPS);
            float w = exp(-t * t * 2.2) * t;
            float m = texture(uMask, uv + dir * t).a;
            sumOuter += m * w;
            sumInner += (1.0 - m) * w;
            sumWeight += w;
        }
    }

    float invWeight = 1.0 / max(sumWeight, 0.0001);
    float outer = sumOuter * invWeight * (1.0 - center);
    float inner = sumInner * invWeight * center;

    float raw = (outer + inner * uInner) * uStrength;
    float glow = 1.0 - exp(-raw * 7.0);
    glow = clamp(glow, 0.0, 1.0);
    if (glow <= 0.002) discard;

    int last = max(uCount - 1, 1);
    float axis = uVertical > 0.5 ? (1.0 - uv.y) : uv.x;
    float pos = clamp(axis, 0.0, 1.0) * float(last);
    int i0 = int(floor(pos));
    int i1 = min(i0 + 1, last);
    vec3 tint = mix(uColors[i0], uColors[i1], fract(pos));

    color = vec4(tint, glow * uAlpha);
}
