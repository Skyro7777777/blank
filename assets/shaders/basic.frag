#version 330 core

in vec3 FragColor;
out vec4 FragOutput;

uniform int isTriangle;

void main() {
    if (isTriangle == 1) {
        // Triangle: use interpolated vertex colors
        FragOutput = vec4(FragColor, 1.0);
    } else {
        // Ground plane: use color as-is with slight darkening
        FragOutput = vec4(FragColor * 0.8, 1.0);
    }
}
