$input v_texcoord0

#include "common.sh"

SAMPLER2D(tex_normal, 0);
SAMPLER2D(tex_depth,  1);

uniform vec4 u_ssao_params; // x: radius, y: bias, z: screenWidth, w: screenHeight
uniform vec4 u_kernel[64];  // Random sample kernel

// x = near plane, y = far plane
uniform vec4 u_camera_params; 

float getLinearDepth(float _rawDepth)
{
    float n = u_camera_params.x;
    float f = u_camera_params.y;
    
    // Standard perspective depth reconstruction
    // This assumes a standard [0, 1] depth buffer (bgfx default)
    return (2.0 * n * f) / (f + n - _rawDepth * (f - n));
}

// Add this procedural noise function
vec3 hash32(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yzz) * p3.zyx);
}

vec3 getViewPos(vec2 _uv, float _depth)
{
    // 1. Convert UV and Depth to Clip Space (NDC)
    // _uv is [0, 1], we need [-1, 1]
    // _depth is usually [0, 1] in bgfx (handled by the backend abstraction)
    vec4 clipPos = vec4(
        _uv.x * 2.0 - 1.0, 
        (1.0 - _uv.y) * 2.0 - 1.0, // Flip Y for render target sampling
        _depth, 
        1.0
    );

    // 2. Multiply by the Inverse Projection Matrix to get View Space
    vec4 viewPos = mul(u_invProj, clipPos);

    // 3. Perspective Divide
    // This transforms the coordinates from Homogeneous space to 3D space
    return viewPos.xyz / viewPos.w;
}

void main() {
    float depth = texture2D(tex_depth, v_texcoord0).r;
    if (depth >= 1.0) discard; // Don't occlude sky

    vec3 normal = texture2D(tex_normal, v_texcoord0).xyz;
    vec3 viewPos = getViewPos(v_texcoord0, depth); // Helper to reconstruct position

    // Replace noise texture lookup with the hash function
    // We use gl_FragCoord.xy so the noise is "locked" to the screen pixels
    vec3 randomVec = normalize(hash32(gl_FragCoord.xy) * 2.0 - 1.0);
    
    // Gram-Schmidt process to create TBN
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i) {
        // Sample position in view space
        vec3 samplePos = TBN * u_kernel[i].xyz;
        samplePos = viewPos + samplePos * u_ssao_params.x;

        // Project sample to get UV coordinates
        vec4 offset = mul(u_proj, vec4(samplePos, 1.0));
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture2D(tex_depth, offset.xy).r;
        float sceneZ = getLinearDepth(sampleDepth);
        
        // Range check to avoid "bleeding" on distant objects
        float rangeCheck = smoothstep(0.0, 1.0, u_ssao_params.x / abs(viewPos.z - sceneZ));
        occlusion += (sceneZ <= samplePos.z + u_ssao_params.y ? 1.0 : 0.0) * rangeCheck;
    }

    float finalOcclusion = 1.0 - (occlusion / 64.0);
    gl_FragColor = vec4(vec3_splat(finalOcclusion), 1.0);
}