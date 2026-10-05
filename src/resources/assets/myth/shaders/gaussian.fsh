#version 330 core

in vec2 uv;
out vec4 color;
uniform sampler2D sourceTexture;
uniform vec2 texelStep;
uniform int radius;
uniform float weights[65];

void main() {
    vec3 result = texture(sourceTexture, uv).rgb * weights[0];
    for (int i = 1; i <= radius; ++i) {
        vec2 offset = texelStep * float(i);
        result += (texture(sourceTexture, uv + offset).rgb
                + texture(sourceTexture, uv - offset).rgb) * weights[i];
    }
    color = vec4(result, 1.0);
}
