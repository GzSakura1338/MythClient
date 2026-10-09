#version 330 core

in vec2 v_TexCoord;
in vec2 v_OneTexel;

uniform sampler2D u_Texture;
uniform int u_ShapeMode;
uniform vec4 u_Fill;
uniform float u_Fill_Offset;
uniform float u_Fill_Strength;
uniform vec4 u_Outline;
uniform float u_Outline_Offset;
uniform float u_Outline_Strength;
uniform int u_Radius;
uniform float u_GlowMultiplier;
uniform bool u_FastLines;
uniform int u_Dots;
uniform int u_DotsRadius;
uniform float u_DotsAlpha;

out vec4 color;

bool decorator() {
    if (u_Dots == 0) return false;
    if (u_Fill.a == 0) return false;
    if (u_Dots == 1)
        return int(gl_FragCoord.x) - (u_DotsRadius * int(gl_FragCoord.x / u_DotsRadius)) == 0
            && int(gl_FragCoord.y) - (u_DotsRadius * int(gl_FragCoord.y / u_DotsRadius)) == 0;
    return int(gl_FragCoord.x) % u_DotsRadius == 0 || int(gl_FragCoord.y) % u_DotsRadius == 0;
}

// Mio's calculate(): rebuild the colour at a shifted hue, keeping its saturation and value.
vec3 shiftHue(vec3 c, float hue) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    vec4 L = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 f = vec3(hue, d / (q.x + e), q.x);
    vec3 r = abs(fract(f.xxx + L.xyz) * 6.0 - L.www);
    return f.z * mix(L.xxx, clamp(r - L.xxx, 0.0, 1.0), f.y);
}

vec4 cycle(vec4 base, float offset, float strength) {
    if (offset == 0.0) return base;
    vec2 spread = v_TexCoord * 3.0 * vec2(-strength, strength);
    float hue = mod(spread.x + spread.y + offset, 1.0);
    return vec4(shiftHue(base.rgb, hue), base.a);
}

void main() {
    vec4 center = texture(u_Texture, v_TexCoord);

    if (center.a != 0.0) {
        if (u_ShapeMode == 0 || u_Fill.a == 0.0) discard;
        vec4 fill = cycle(u_Fill, u_Fill_Offset, u_Fill_Strength);
        center = decorator() ? vec4(fill.rgb, u_DotsAlpha) : fill;
    } else {
        if (u_ShapeMode == 1 || u_Radius == 0) discard;

        float dist = u_Radius * u_Radius + 1.0;

        if (u_FastLines && u_Radius > 2) {
            bool present = false;
            for (int x = -u_Radius; x <= u_Radius; x += u_Radius) {
                for (int y = -u_Radius; y <= u_Radius; y += u_Radius) {
                    if (texture(u_Texture, v_TexCoord + v_OneTexel * vec2(x, y)).a != 0.0) {
                        present = true;
                        break;
                    }
                }
            }
            if (!present) discard;
        }

        for (int x = -u_Radius; x <= u_Radius; x++) {
            for (int y = -u_Radius; y <= u_Radius; y++) {
                if (texture(u_Texture, v_TexCoord + v_OneTexel * vec2(x, y)).a != 0.0)
                    dist = min(x * x + y * y - 1.0, dist);
            }
        }

        float minDist = u_Radius * u_Radius;

        if (dist > minDist) discard;
        vec4 outline = cycle(u_Outline, u_Outline_Offset, u_Outline_Strength);
        center.rgb = outline.rgb;
        center.a = min((1.0 - (dist / minDist)) * u_GlowMultiplier, 1.0) * outline.a;
        if (dist <= 1.5 && u_GlowMultiplier > 0) center.a = 0.8 * outline.a;
    }

    color = center;
}
