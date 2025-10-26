#version 450

#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : enable

const int MAX_LIGHTS = 8;

// Set 0: Global Data — lights (should be an array)
layout(set = 0, binding = 1) uniform Light {
    mat4 light_projview;
    float light_nearclip;
    float light_range;
} lights[MAX_LIGHTS]; // Example array with a defined size

layout(push_constant) uniform PushConstants {
    mat4 model;
    int lightindex;
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

layout(location = 6) out float fraglight_nearclip;
layout(location = 7) out float fraglight_range;
layout(location = 8) out mat4 fraglight_projview;

void main() {
    gl_Position = lights[lightindex].light_projview * model * vec4(inPosition, 1.0);
    fraglight_projview = lights[lightindex].light_projview;
    fraglight_nearclip = lights[lightindex].light_nearclip;
    fraglight_range = lights[lightindex].light_range;

    fragPos = (model * vec4(inPosition, 1.0)).xyz;
    fragTexCoord = inTexCoord;
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vertColor = vec4(inColor, 1.0f);

    // TBN matrix calculation
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 B = normalize(cross(N, T));

    T = normalize(T - dot(T, N) * N);
    
    // Pass TBN vectors to fragment shader
    fragT = T;
    fragB = B;
    fragN = N;
}