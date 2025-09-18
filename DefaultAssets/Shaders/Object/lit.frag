#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 vertColor;
layout(location = 3) in vec3 fragT;
layout(location = 4) in vec3 fragB;
layout(location = 5) in vec3 fragN;
layout(location = 6) in vec3 camera_pos;


//Set 2: Textures
layout(set = 2, binding = 0) uniform Flags {
    int has_color_tex;
    int has_normal_tex;
    int has_arm_tex;
};
layout(set = 2, binding = 1) uniform sampler2D color_tex;
layout(set = 2, binding = 2) uniform sampler2D normal_tex;
layout(set = 2, binding = 3) uniform sampler2D arm_tex;

//=================LIGHTING====================//
const int MAX_LIGHTS = 10;

// Shadowmaps (should be an array)
layout(set = 2, binding = 4) uniform sampler2D shadow_tex[MAX_LIGHTS]; // Example array

// Set 0: Global Data — lights (should be an array)
layout(set = 0, binding = 1) uniform Light {
    vec3 light_color;
    vec3 light_direction;
    int light_type;
    float light_range;
} lights[MAX_LIGHTS]; // Example array with a defined size

//Set 1: Object Data
layout(set = 1, binding = 1) uniform Shading {
    vec3 color;
    vec3 tint;
    float metallic;
};

layout(location = 0) out vec4 outColor;

void main() {
    //texture calcs
    vec3 _color_fromtex = texture(color_tex, fragTexCoord).xyz * color;

    //locals
    vec3 _color;
    if(has_color_tex > 0) //ALBEDO TEXTURE
        _color = _color_fromtex;
    else
        _color = color;
    vec3 _tint = tint;
    vec3 _normal = fragN;
    if (has_normal_tex > 0) { //NORMAL TEXTURE
        vec3 normal_from_map = texture(normal_tex, fragTexCoord).rgb * 2.0 - 1.0;
        mat3 TBN = mat3(fragT, fragB, fragN);
        _normal = TBN * normal_from_map;
    }
    float _metallic = metallic;
    float _roughness = 0.5;
    if(has_arm_tex > 0) { //ARM TEXTURE
        vec3 arm_data = texture(arm_tex, fragTexCoord).rgb;
        _roughness = arm_data.g;
        _metallic = arm_data.b; 
    }
    _normal = normalize(_normal);

    vec3 final_result = _tint;

    // Loop through each light
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        // Lighting
        vec3 lightdelta = normalize(-lights[i].light_direction);

        // Diffuse component
        float diff = max(dot(_normal, lightdelta), 0.0);
        vec3 diffuse = _color * diff * lights[i].light_color; // Use the light's color

        // Specular component
        vec3 viewDir = normalize(camera_pos - fragPos);
        vec3 reflectDir = reflect(-lightdelta, _normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - _roughness) * 128.0 + 1.0);
        vec3 specular = vec3(spec, spec, spec) * _metallic * lights[i].light_color;

        final_result += diffuse + specular;

        // NOTE: You would add shadow mapping calculations here
        // e.g., sample from shadow_tex[i]
    }

    outColor = vec4(final_result, 1.0);
}