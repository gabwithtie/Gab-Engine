#version 450

#extension GL_ARB_shader_viewport_layer_array : require

layout(set = 0, binding = 0) uniform Global {
    mat4 view;
    mat4 proj;
    vec3 camera_pos;
} global;

layout(set = 1, binding = 0) uniform Object {
    mat4 model;
};

layout(push_constant) uniform PushConstants {
    int layerIndex;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 vertColor;
layout(location = 3) out vec3 fragT;
layout(location = 4) out vec3 fragB;
layout(location = 5) out vec3 fragN;

layout(location = 6) out vec3 camera_pos;

void main() {
    gl_Position = global.proj * global.view * model * vec4(inPosition, 1.0);
    float max_z = gl_Position.w - 0.01;
    gl_Position.z = min(max_z, gl_Position.z);

    fragPos = (model * vec4(inPosition, 1.0)).xyz;
    vertColor = vec4(inColor, 1.0f);

    camera_pos = global.camera_pos;
}