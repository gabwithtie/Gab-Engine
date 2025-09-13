#version 450

// Renders a full-screen triangle without any buffers
layout(location = 0) out vec2 fragTexCoord;

void main() {
    fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragTexCoord * 2.0 - 1.0, 0.0, 1.0);
}
