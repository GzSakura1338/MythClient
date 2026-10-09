#version 330 core

in vec2 v_TexCoord;
in vec2 v_OneTexel;

uniform sampler2D u_Texture;
uniform int u_Width;
uniform bool u_FastLines;
uniform int u_ShapeMode;
uniform float u_GlowMultiplier;
uniform vec4 u_FillColor;
uniform vec4 u_OutlineColor;
uniform int u_Dots;
uniform int u_DotsRadius;
uniform float u_DotsAlpha;

out vec4 color;

bool decorator() {
    if (u_Dots == 0) return false;
    if (u_FillColor.a == 0) return false;
    if (u_Dots == 1)
        return int(gl_FragCoord.x) - (u_DotsRadius * int(gl_FragCoord.x / u_DotsRadius)) == 0
            && int(gl_FragCoord.y) - (u_DotsRadius * int(gl_FragCoord.y / u_DotsRadius)) == 0;
    return int(gl_FragCoord.x) % u_DotsRadius == 0 || int(gl_FragCoord.y) % u_DotsRadius == 0;
}

void main() {
    vec4 center = texture(u_Texture, v_TexCoord);

    if (center.a != 0.0) {
        if (u_ShapeMode == 0) discard;
        if (decorator()) center = vec4(u_FillColor.rgb, u_DotsAlpha);
        else center = u_FillColor;
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
                vec4 offset = texture(u_Texture, v_TexCoord + v_OneTexel * vec2(x, y));
                if (offset.a != 0.0) {
                    dist = min(x * x + y * y - 1.0, dist);
                    center = offset;
                }
            }
        }

        float minDist = u_Width * u_Width;

        if (dist > minDist) center.a = 0.0;
        else {
            center.rgb = u_OutlineColor.rgb;
            center.a = min((1.0 - (dist / minDist)) * u_GlowMultiplier, 1.0);
            if (dist <= 1.5 && u_GlowMultiplier > 0) center.a = 0.8;
        }
    }

    color = center;
}
