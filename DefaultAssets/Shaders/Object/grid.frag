#version 450

//S1B0 already used by Object in vertex shader
layout(set = 1, binding = 1) uniform Object_Shading {
    vec4 color_a;
    vec4 color_b;
};

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 vertColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 size = vec2(15, 15);

    float total = floor(fragTexCoord.x * float(size.x)) +
                  floor(fragTexCoord.y * float(size.y));
    bool isEven = mod(total, 2.0) == 0.0;
    outColor = (isEven) ? color_a : color_b;
}