#version 450


layout(set = 0, binding = 0) uniform Global {
    mat4 view;
    mat4 proj;

    mat4 view_sun;

    vec3 camera_pos;
};

layout(set = 1, binding = 0) uniform Object {
    mat4 model;
};


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 vertColor;

layout(location = 4) out vec4 fragPos_sun;

layout(location = 5) out vec3 out_camera_pos;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);

    fragPos = (model * vec4(inPosition, 1.0)).xyz;
    fragTexCoord = inTexCoord;
    fragNormal = inNormal;
    vertColor = vec4(inColor, 1.0f);

    fragPos_sun = view_sun * model * vec4(inPosition, 1.0);

    out_camera_pos = camera_pos;
}