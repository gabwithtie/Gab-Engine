#version 450

//TEXTURES

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 3) in vec4 vertColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vertColor;
}