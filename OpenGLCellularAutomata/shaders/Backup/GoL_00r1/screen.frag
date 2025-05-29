#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

uniform usampler2D uTexture;

void main() {
    uvec4 cell = texture(uTexture, TexCoord);

    if (cell.r == 1u) {
        FragColor = vec4(1.0);                      // white material = alive
    } else if (cell.r == 2u) {
        FragColor = vec4(0.0, 0.4, 1.0, 1.0);       // blue material
    } else if (cell.r == 3u) {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0);       // yellow material
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);       // black = dead or empty
    }
}