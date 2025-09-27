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
    mat4 rotView = global.view;
    rotView[3] = vec4(0.0, 0.0, 0.0, 1.0);

    gl_Position = global.proj * rotView * vec4(inPosition, 1.0);
    gl_Position.z = gl_Position.w;
    gl_Position.z -= 0.001;

    fragPos = (model * vec4(inPosition, 1.0)).xyz;
}