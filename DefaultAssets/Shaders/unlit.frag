#version 450

layout(set = 1, binding = 0) uniform sampler2D colortex;

//TEXTURES
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(colortex, fragTexCoord);
}