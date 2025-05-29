#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

uniform usampler2D cellTexture;  // Match the uniform name in main.cpp

void main() {
    uvec3 cell = texture(cellTexture, TexCoord).rgb;  // GL_RGB8UI returns uvec3

    if (cell.r == 1u) {
        FragColor = vec4(1.0); // alive = white
    } else if (cell.r == 2u) {
        FragColor = vec4(0.0, 0.4, 1.0, 1.0); // blue material
    } else if (cell.r == 3u) {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // yellow material
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // dead = black
    }
}