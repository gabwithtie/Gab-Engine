#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 vertColor;

layout(location = 4) in vec4 fragPos_sun;

layout(location = 5) in vec3 camera_pos;

layout(set = 2, binding = 0) uniform sampler2D color_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D specular_tex;
layout(set = 2, binding = 3) uniform sampler2D ao_tex;
layout(set = 2, binding = 4) uniform sampler2D shadow_map;

//Set 0: Global Data
layout(set = 0, binding = 1) uniform Light {
    vec3 light_color;
    vec3 light_direction;
};

//Set 1: Object Data
layout(set = 1, binding = 1) uniform Shading {
    vec3 color;
    vec3 tint;
    float specular;
};

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normalizedNormal = normalize(fragNormal);
    vec3 lightDir = normalize(-light_direction); // Directional light vector points away from the light
    
    //Ambient component
    vec3 ambient = tint;

    // Diffuse component
    float diff = max(dot(normalizedNormal, lightDir), 0.0);
    vec3 diffuse = color * diff;
    
    // Specular component
    vec3 viewDir = normalize(camera_pos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normalizedNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specular);
    vec3 specular = vec3(spec, spec, spec);
    
    // Final color
    vec3 result = ambient + diffuse + specular;
    outColor = vec4(1,1,1, 1.0);
}