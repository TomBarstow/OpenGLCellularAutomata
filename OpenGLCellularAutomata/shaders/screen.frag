#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform usampler2D cellTexture;

void main() {
    uint cellState = texture(cellTexture, TexCoord).r;
    
    if (cellState > 0u) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);  // White for alive cells
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);  // Black for dead cells
    }
}