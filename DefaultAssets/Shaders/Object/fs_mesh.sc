$input v_pos, v_view, v_normal, v_color0, v_texcoord0, v_tangent, v_bitangent
/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bgfx_shader.sh>
#include "../shaderlib.sh"

#define MAX_LIGHTS 10

//Material stuff

uniform vec4 color;
uniform vec4 metallic;
#define metallic metallic.x

uniform vec4 has_color_tex;
#define has_color_tex has_color_tex.x
SAMPLER2D(color_tex, 1); 
uniform vec4 has_normal_tex;
#define has_normal_tex has_normal_tex.x
SAMPLER2D(normal_tex, 2);
uniform vec4 has_arm_tex;
#define has_arm_tex has_arm_tex.x
SAMPLER2D(arm_tex, 3);

uniform vec4 tiling;

//Camera buffers
SAMPLER2D(tex_ao, 4);


//Lights
SAMPLER2DARRAY(light_map, 0);
uniform mat4 light_view[MAX_LIGHTS];
uniform mat4 light_proj[MAX_LIGHTS];
uniform vec4 light_pos[MAX_LIGHTS];
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
    bool outside = any(greaterThan(uv.xy, vec2(1.0, 1.0))) 
                || any(lessThan(uv.xy, vec2(0.0, 0.0)));

    if (outside) {
        return 1.0; // Return "No Shadow" (1.0 means fully lit)
    }

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

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	float ndotl = dot(_normal, _lightDir);
	//vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	vec3 reflected = 2.0*ndotl*_normal - _lightDir;
	float rdotv = dot(reflected, _viewDir);
	return vec2(ndotl, rdotv);
}

float fresnel(float _ndotl, float _bias, float _pow)
{
	float facing = (1.0 - _ndotl);
	return max(_bias + (1.0 - _bias) * pow(facing, _pow), 0.0);
}

vec4 lit(float _ndotl, float _rdotv, float _m)
{
	float diff = max(0.0, _ndotl);
	float spec = step(0.0, _ndotl) * max(0.0, _rdotv * _m);
	return vec4(1.0, diff, spec, 1.0);
}

vec4 powRgba(vec4 _rgba, float _pow)
{
	vec4 result;
	result.xyz = pow(_rgba.xyz, vec3_splat(_pow) );
	result.w = _rgba.w;
	return result;
}

vec3 perturbNormal(vec3 _wpos, vec3 _normal, vec2 _uv, sampler2D _normalTex) {
    // 1. Get Normal Map components (X and Y only)
    vec3 normalSample = texture2D(_normalTex, _uv).xyz * 2.0 - 1.0;
    
    // 2. Get the surface derivatives
    vec3 dp1 = dFdx(_wpos);
    vec3 dp2 = dFdy(_wpos);
    vec2 duv1 = dFdx(_uv);
    vec2 duv2 = dFdy(_uv);

    // 3. Solve the linear system for Tangent and Bitangent
    vec3 dp2perp = cross(dp2, _normal);
    vec3 dp1perp = cross(_normal, dp1);
    
    // This creates a coordinate system based purely on UV flow
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // 4. Construct the Surface Gradient
    // We normalize the basis vectors to handle scaling/distortion
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    
    // Surface Gradient accumulation: Normal = normalize(N_geom - Gradient)
    // We force the Z to be positive to ensure it points out
    vec3 res = normalize(_normal - invmax * (normalSample.x * T + normalSample.y * B));
    
    // Guard: ensure it stays on the forward hemisphere
    return res * sign(dot(res, _normal) + 0.0001);
}

void main() {
    vec3 albedo = color.xyz;
    vec2 final_texuv = v_texcoord0;
    final_texuv *= tiling.xy;

    if (has_color_tex > 0.5) {
        albedo *= toLinear(texture2D(color_tex, final_texuv) );;
    }

    // 1. Geometric Normal and View Direction (World Space)
    vec3 geoNormal = normalize(v_normal);
    vec3 view = normalize(v_view);
    vec3 normal = geoNormal;

    // 2. Perturb Normal using Surface Gradients
    if (has_normal_tex > 0.5) {
        normal = perturbNormal(v_pos, geoNormal, final_texuv, normal_tex);
    }

    float _matAO = 1.0;
    float _roughness = 0.5; // Default middle-ground
    float _metallic = metallic;  // Default non-metal
    
    if (has_arm_tex > 0.5) {
        vec3 arm = texture2D(arm_tex, final_texuv).rgb;
        _matAO = arm.r;
        _roughness = arm.g;
        _metallic = arm.b;
    }
    
    vec3 lightColor = vec3(0 ,0 ,0);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        vec3 lightDir;
        float attenuation = 1.0;
        float shadow = 1.0;

        // Calculate direction and attenuation based on light type
        if (abs(light_type[i].x) < 0.5) { // Directional
            // Assuming view matrix forward vector is the direction
            lightDir = normalize(light_view[i][2].xyz); 
            shadow = getShadow(i, v_pos, normal, lightDir, light_bias_min[i].x, light_bias_mult[i].x);
        } 
        else { // Point or Spot
            vec3 lightPos = light_pos[i].xyz; // Extract position from view matrix
            vec3 disp = lightPos - v_pos;
            float dist = length(disp);
            lightDir = normalize(disp);
            
            // Basic inverse-square falloff with range clamp
            attenuation = saturate(1.0 - (dist / light_range[i].x));
            attenuation *= attenuation;

            if (abs(light_type[i].x - 1) < 0.5) { // Spot Light
                vec3 spotDir = normalize(light_view[i][2].xyz);
                float cosAngle = dot(lightDir, spotDir);
                float inner = cos(light_cone_inner[i].x / 2);
                float outer = cos(light_cone_outer[i].x / 2);
                attenuation *= saturate((cosAngle - outer) / (inner - outer));

                shadow = getShadow(i, v_pos, normal, lightDir, light_bias_min[i].x, light_bias_mult[i].x);
            }
        }

	    lightDir = lightDir;
	    vec2 bln = blinn(lightDir, normal, view);
	    vec4 lc = lit(bln.x, bln.y, 1.0);
	    vec3 rgb = saturate(lc.y) * attenuation;

        lightColor += light_color[i].xyz * rgb * shadow;
    }

    // Sample SSAO using screen coordinates
    vec2 screenUV = gl_FragCoord.xy / u_viewRect.zw;
    float combinedAO = _matAO * max(texture2D(tex_ao, screenUV).r, 0.1f);

    ////////
	gl_FragColor.xyz = max(vec3_splat(0.05), lightColor.xyz) * albedo.xyz * combinedAO;
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
}