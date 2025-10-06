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
const int MAX_LIGHTS = 5;
// Light Types
const int LIGHT_TYPE_DIRECTIONAL = 0;
const int LIGHT_TYPE_SPOT = 1;

// Shadowmaps (should be an array)
layout(set = 2, binding = 4) uniform sampler2DArray shadow_tex;

// Set 0: Global Data — lights (should be an array)
layout(set = 0, binding = 1) uniform Light {
    mat4 light_view;
    mat4 light_proj;
    vec3 light_color;
    int light_type;
    float light_range;
    float light_cone_inner;
    float light_cone_outer;
    float bias_min;
    float bias_mult;
} lights[MAX_LIGHTS]; // Example array with a defined size

//Set 1: Object Data
layout(set = 1, binding = 1) uniform Shading {
    vec3 color;
    vec3 tint;
    float metallic;
    float shadow_strength;
};

layout(location = 0) out vec4 outColor;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );


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

    // --- Lighting Loop ---
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        vec3 lightDir;
        float attenuation = 1.0;

        if (lights[i].light_type == LIGHT_TYPE_DIRECTIONAL) {
            lightDir = normalize(vec3(lights[i].light_view[0][2], lights[i].light_view[1][2], lights[i].light_view[2][2]));
        } else if (lights[i].light_type == LIGHT_TYPE_SPOT) {
            mat4 inverse_light_view = inverse(lights[i].light_view);
            vec3 light_pos = inverse_light_view[3].xyz;
            vec3 L_to_P = light_pos - fragPos;
            float dist = length(L_to_P);
            
            // Range attenuation (simple inverse square falloff adjusted by light_range)
            attenuation = 1.0 / (1.0 + ((dist * dist) / (lights[i].light_range * lights[i].light_range)));

            lightDir = normalize(L_to_P);
            
            // Light's forward direction (negative Z-axis of the light's view matrix)
            vec3 spotDir = normalize(vec3(lights[i].light_view[0][2], lights[i].light_view[1][2], lights[i].light_view[2][2]));
            
            // Dot product of the direction *to* the light, and the spot's *direction*
            float theta = acos(dot(lightDir, spotDir));
            
            // Calculate cone intensity (smoothstep interpolation)
            float c_outer = radians(lights[i].light_cone_outer * 0.5);
            float c_inner = radians(lights[i].light_cone_inner * 0.5);
            float intensity = 0.0;
            if (theta < c_outer) // For smooth falloff
            {
                intensity = smoothstep(c_outer, c_inner, theta);
            }
            if (theta < c_inner)
            {
                intensity = 1.0; // Inside the main cone
            }

            // Combine range attenuation with cone intensity
            attenuation *= intensity;
        }

        // Diffuse component
        float diff = max(dot(_normal, lightDir), 0.0);
        vec3 diffuse = _color * diff * lights[i].light_color;

        // Specular component
        vec3 viewDir = normalize(camera_pos - fragPos);
        vec3 reflectDir = reflect(-lightDir, _normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), (1.0 - _roughness) * 128.0 + 1.0);
        vec3 specular = vec3(spec) * _metallic * lights[i].light_color;

        // *** SHADOW MAPPING LOGIC ***
        vec4 fragPosLightSpace = lights[i].light_proj * lights[i].light_view * vec4(fragPos, 1.0);
		vec3 shadowcoord = fragPosLightSpace.xyz / fragPosLightSpace.w;
        shadowcoord = shadowcoord * 0.5 + 0.5;

        ivec3 texDim = textureSize(shadow_tex, 0);
	    float scale = 1.5;
	    float dx = scale * 1.0 / float(texDim.x);
	    float dy = scale * 1.0 / float(texDim.y);

	    float shadow = 0.0;
	    float count = 0;
	
	    for (int x = -1; x <= 1; x++)
	    {
		    for (int y = -1; y <= 1; y++)
		    {
                float closest = texture(shadow_tex, vec3(shadowcoord.xy + vec2(dx*x, dy*y), i)).r;
                float bias = max(lights[i].bias_mult * (dot(_normal, lightDir)), lights[i].bias_min);
                
                shadow += (shadowcoord.z - bias) < closest ? 1.0 : 0.0;

			    count++;
		    }
	    }

	    shadow = shadow / count;

        // REVISED: Apply shadow factor to lighting
        vec3 sub_final = (diffuse + specular) * attenuation;
        final_result += mix(sub_final, sub_final * shadow, shadow_strength);
    }

    outColor = vec4(final_result, 1.0);
}