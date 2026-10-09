#version 330 core

in vec2 v_TexCoord;
in vec2 v_OneTexel;

uniform sampler2D u_Texture;
uniform int u_Width;
uniform bool u_FastLines;
uniform int u_ShapeMode;
uniform float u_GlowMultiplier;
uniform float u_Time;
uniform float u_Step;
uniform vec4 u_FillColor;
uniform vec4 u_FillColor2;
uniform vec4 u_OutlineColor;
uniform vec4 u_OutlineColor2;
uniform int u_Dots;
uniform int u_DotsRadius;
uniform float u_DotsAlpha;

out vec4 color;

bool decorator(vec4 c) {
    if (u_Dots == 0) return false;
    if (c.a == 0) return false;
    if (u_Dots == 1)
        return int(gl_FragCoord.x) - (u_DotsRadius * int(gl_FragCoord.x / u_DotsRadius)) == 0
            && int(gl_FragCoord.y) - (u_DotsRadius * int(gl_FragCoord.y / u_DotsRadius)) == 0;
    return int(gl_FragCoord.x) % u_DotsRadius == 0 || int(gl_FragCoord.y) % u_DotsRadius == 0;
}

// Mio measures from the screen origin, so the bands sweep across the view rather than
// sticking to the entity. Kept as it was; it is what the effect looks like over there.
vec4 gradient(vec4 a, vec4 b) {
    vec2 frag = gl_FragCoord.xy;
    float d = sqrt(frag.x * frag.x + frag.y * frag.y) / u_Step;
    d = (sin(d + u_Time) + 1.0) / 2.0;
    return a * d + b * (1.0 - d);
}

void main() {
    vec4 center = texture(u_Texture, v_TexCoord);

    if (center.a != 0.0) {
        if (u_ShapeMode == 0) discard;
        vec4 fill = gradient(u_FillColor, u_FillColor2);
        center = decorator(fill) ? vec4(fill.rgb, u_DotsAlpha) : fill;
    } else {
        if (u_ShapeMode == 1 || u_Width == 0) discard;

        float dist = u_Width * u_Width + 1.0;

        if (u_FastLines && u_Width > 2) {
            bool present = false;
            for (int x = -u_Width; x <= u_Width; x += u_Width) {
                for (int y = -u_Width; y <= u_Width; y += u_Width) {
                    if (texture(u_Texture, v_TexCoord + v_OneTexel * vec2(x, y)).a != 0.0) {
                        present = true;
                        break;
                    }
                }
            }
            if (!present) discard;
        }

        for (int x = -u_Width; x <= u_Width; x++) {
            for (int y = -u_Width; y <= u_Width; y++) {
                if (texture(u_Texture, v_TexCoord + v_OneTexel * vec2(x, y)).a != 0.0)
                    dist = min(x * x + y * y - 1.0, dist);
            }
        }

        float minDist = u_Width * u_Width;

        if (dist > minDist) discard;
        vec4 outline = gradient(u_OutlineColor, u_OutlineColor2);
        center.rgb = outline.rgb;
        center.a = min((1.0 - (dist / minDist)) * u_GlowMultiplier, 1.0) * outline.a;
        if (dist <= 1.5 && u_GlowMultiplier > 0) center.a = 0.8 * outline.a;
    }

    color = center;
}
