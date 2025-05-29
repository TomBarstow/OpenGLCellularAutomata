#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;

void main() {
    uvec4 cell = texture(uTexture, TexCoord).rgba;

    if (cell.r == 1u) {
        FragColor = vec4(1.0); // alive = white
    } else if (cell.r == 2u) {
        FragColor = vec4(0.0, 0.4, 1.0, 1.0); // blue material
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // dead = black
    }
}
