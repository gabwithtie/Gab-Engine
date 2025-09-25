#version 450

//TEXTURES

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 vertColor;
layout(location = 3) in vec3 fragT;
layout(location = 4) in vec3 fragB;
layout(location = 5) in vec3 fragN;
layout(location = 6) in vec3 camera_pos;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(gl_FragCoord.z, 0, 0,1);
}