$input v_texcoord0

#include "common.sh"

SAMPLER2D(s_normal, 0);
SAMPLER2D(s_depth,  1);
SAMPLER2D(s_noise,  2); // 4x4 tiling noise texture

uniform vec4 u_ssao_params; // x: radius, y: bias, z: screenWidth, w: screenHeight
uniform vec4 u_kernel[64];  // Random sample kernel

void main() {
    float depth = texture2D(s_depth, v_texcoord0).r;
    if (depth >= 1.0) discard; // Don't occlude sky

    vec3 normal = texture2D(s_normal, v_texcoord0).xyz;
    vec3 viewPos = getViewPos(v_texcoord0, depth); // Helper to reconstruct position

    // Create a TBN matrix to rotate the kernel around the normal
    vec2 noiseScale = vec2(u_ssao_params.z/4.0, u_ssao_params.w/4.0);
    vec3 randomVec = texture2D(s_noise, v_texcoord0 * noiseScale).xyz;
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

        float sampleDepth = texture2D(s_depth, offset.xy).r;
        float sceneZ = getLinearDepth(sampleDepth);
        
        // Range check to avoid "bleeding" on distant objects
        float rangeCheck = smoothstep(0.0, 1.0, u_ssao_params.x / abs(viewPos.z - sceneZ));
        occlusion += (sceneZ <= samplePos.z + u_ssao_params.y ? 1.0 : 0.0) * rangeCheck;
    }

    float finalOcclusion = 1.0 - (occlusion / 64.0);
    gl_FragColor = vec4(vec3_splat(finalOcclusion), 1.0);
}