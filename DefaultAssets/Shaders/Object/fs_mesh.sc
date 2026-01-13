$input v_pos, v_view, v_normal, v_color0, v_texcoord0, v_tangent, v_bitangent
/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bgfx_shader.sh>

#define MAX_LIGHTS 10

//Material stuff

uniform vec4 color;
uniform float metallic;

uniform vec4 has_color_tex;
#define has_color_tex has_color_tex.x
SAMPLER2D(color_tex, 1); 
uniform vec4 has_normal_tex;
#define has_normal_tex has_normal_tex.x
SAMPLER2D(normal_tex, 2);
uniform vec4 has_arm_tex;
#define has_arm_tex has_arm_tex.x
SAMPLER2D(arm_tex, 3);

//Camera buffers
SAMPLER2D(tex_ao, 4);

//Lights
SAMPLER2DARRAY(light_map, 0);
uniform mat4 light_pos[MAX_LIGHTS];
uniform mat4 light_view[MAX_LIGHTS];
uniform mat4 light_proj[MAX_LIGHTS];
uniform vec4 light_color[MAX_LIGHTS];
uniform vec4 light_type[MAX_LIGHTS];
uniform vec4 light_is_square[MAX_LIGHTS];
uniform vec4 light_nearclip[MAX_LIGHTS];
uniform vec4 light_range[MAX_LIGHTS];
uniform vec4 light_bias_min[MAX_LIGHTS];
uniform vec4 light_bias_mult[MAX_LIGHTS];
uniform vec4 light_cone_inner[MAX_LIGHTS];
uniform vec4 light_cone_outer[MAX_LIGHTS];

const vec2 s_poissonDisk[16] = 
{
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
};

// High-performance pseudo-random function for dithering
float random(vec3 seed, float i) {
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float getShadow(int _layer, vec3 _wpos, vec3 _normal, vec3 _lightDir, float _minBias, float _multBias)
{
    mat4 shadowMtx = mul(light_proj[_layer], light_view[_layer]);
    vec4 shadowProj = mul(shadowMtx, vec4(_wpos, 1.0));
    vec3 shadowCoord = shadowProj.xyz / shadowProj.w;
    
    vec2 uv = shadowCoord.xy * 0.5 + 0.5;
    
    #if BGFX_SHADER_LANGUAGE_HLSL || BGFX_SHADER_LANGUAGE_PSSL || BGFX_SHADER_LANGUAGE_SPIRV
        uv.y = 1.0 - uv.y;
    #endif

    float currentDepth = shadowCoord.z;
    float bias = max(_multBias * (1.0 - dot(_normal, _lightDir)), _minBias);

    // Controls the softness of the shadow
    float filterRadius = 1.0 / 2048.0; 
    float shadow = 0.0;

    // Use a random rotation per-pixel to hide patterns
    float angle = random(_wpos, float(_layer)) * 2.0 * 3.14159;
    float s = sin(angle);
    float c = cos(angle);
    mat2 rotation = mat2(c, -s, s, c);

    for (int i = 0; i < 16; i++)
    {
        // Rotate the Poisson sample and scale by our filter radius
        vec2 offset = mul(rotation, s_poissonDisk[i]) * filterRadius;
        
        float pcfDepth = texture2DArray(light_map, vec3(uv + offset, float(_layer))).r;
        shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
    }

    return shadow / 16.0;
}

void main() {
    vec3 albedo = color.xyz;
    if (has_color_tex > 0.5) {
        albedo *= texture2D(color_tex, v_texcoord0).xyz;
    }

    vec3 normal = normalize(v_normal);
    if (has_normal_tex > 0.5) {
        vec3 normalSample = texture2D(normal_tex, v_texcoord0).xyz * 2.0 - 1.0;
        
        mat3 TBN = mat3(
            normalize(v_tangent),
            normalize(v_bitangent),
            normal
        );
        //normal = normalize(mul(TBN, normalSample));
    }

    float _matAO = 1.0;
    float _roughness = 0.5; // Default middle-ground
    float _metallic = metallic;  // Default non-metal
    
    if (has_arm_tex > 0.5) {
        vec3 arm = texture2D(arm_tex, v_texcoord0).rgb;
        _matAO = arm.r;
        _roughness = arm.g;
        _metallic = arm.b;
    }

    vec3 viewDir = normalize(v_view);
    vec3 finalDiffuse = vec3(0.0, 0.0, 0.0);
    vec3 finalSpecular = vec3(0.0, 0.0, 0.0);
    
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        vec3 lightDir;
        float attenuation = 1.0;

        // Calculate direction and attenuation based on light type
        if (light_type[i].x == 0) { // Directional
            // Assuming view matrix forward vector is the direction
            lightDir = normalize(light_view[i][2].xyz); 
        } 
        else { // Point or Spot
            vec3 lightPos = light_pos[i].xyz; // Extract position from view matrix
            vec3 disp = lightPos - v_pos;
            float dist = length(disp);
            lightDir = normalize(disp);
            
            // Basic inverse-square falloff with range clamp
            attenuation = saturate(1.0 - (dist / light_range[i].x));
            attenuation *= attenuation;

            if (light_type[i].x == 1) { // Spot Light
                vec3 spotDir = normalize(light_view[i][2].xyz);
                float cosAngle = dot(-lightDir, spotDir);
                float inner = cos(light_cone_inner[i].x);
                float outer = cos(light_cone_outer[i].x);
                attenuation *= saturate((cosAngle - outer) / (inner - outer));
            }
        }

        // Diffuse (Lambert)
        float diff = max(dot(normal, lightDir), 0.0);
        // --- Specular (Simplified Blinn-Phong for PBR-ish look) ---
        vec3 halfDir = normalize(lightDir + viewDir);
        float specPower = pow(8192.0, 1.0 - _roughness); // Map roughness to shininess
        float spec = pow(max(dot(normal, halfDir), 0.0), specPower);

        // Shadow Calculation
        float shadow = getShadow(i, v_pos, normal, lightDir, light_bias_min[i].x, light_bias_mult[i].x);

        // Metallic colors the specular reflection with albedo
        vec3 specColor = lerp(vec3(0.04), albedo, _metallic);
        
        finalDiffuse += light_color[i].xyz * diff * attenuation;
        finalSpecular += light_color[i].xyz * spec * attenuation * shadow * specColor;
    }

    // Ambient light baseline
    finalDiffuse += vec3(0.2f);

    // Sample SSAO using screen coordinates
    vec2 screenUV = gl_FragCoord.xy / u_viewRect.zw;
    float combinedAO = _matAO * max(texture2D(tex_ao, screenUV).r, 0.1f);

    vec3 diffuseResult = finalDiffuse * albedo * (1.0 - _metallic) * combinedAO;
    vec3 ambientResult = vec3(0.05) * albedo;

    vec3 result = ambientResult + (diffuseResult + finalSpecular);

    result = clamp(result, 0.0, 1.0);
    gl_FragColor = vec4(result, 1.0);
}