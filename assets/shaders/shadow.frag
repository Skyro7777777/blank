#version 330 core

// Shadow pass - only depth is written
// This shader exists only to satisfy the shader program linker
void main() {
    // gl_FragDepth is automatically set from the depth buffer
}
