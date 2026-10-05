#version 330 core

precision mediump float;

out vec4 color;

uniform vec4 uColor;

void main() {
    color = uColor;
}
