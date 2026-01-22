$input v_texcoord0

#include "common.sh"

SAMPLER2D(s_tex_input, 0); 
SAMPLER2D(s_tex_depth, 1); 

// x: near, y: far, z: screenWidth, w: screenHeight
uniform vec4 u_camera_params; 
// x: blur radius, y: sharpness, z: dirX, w: dirY
uniform vec4 u_blur_params;   

float getLinearDepth(float _rawDepth)
{
    float n = u_camera_params.x;
    float f = u_camera_params.y;
    return (2.0 * n * f) / max(0.00001, (f + n - _rawDepth * (f - n)));
}

void main() {
    vec2 invScreen = 1.0 / max(u_camera_params.zw, vec2(1.0, 1.0));
    vec2 direction = u_blur_params.zw;
    float radius = u_blur_params.x;
    float sharpness = u_blur_params.y;

    float centerRawDepth = texture2D(s_tex_depth, v_texcoord0).r;
    if (centerRawDepth >= 1.0) {
        gl_FragColor = texture2D(s_tex_input, v_texcoord0);
        return;
    }

    float centerDepth = getLinearDepth(centerRawDepth);
    float centerValue = texture2D(s_tex_input, v_texcoord0).r;

    float totalWeight = 1.0;
    float totalValue = centerValue;

    // We use a sigma that scales with our radius to keep the curve smooth
    float sigma = radius * 0.5;
    float twoSigmaSq = 2.0 * sigma * sigma;

    // 17-tap kernel (-8 to 8)
    for (float i = -8.0; i <= 8.0; i += 1.0) {
        if (i == 0.0) continue;

        float noise = fract(sin(dot(v_texcoord0 + i, vec2(12.9898, 78.233))) * 43758.5453);
        vec2 offset = direction * (i + noise - 0.5) * invScreen * (radius / 8.0);

        vec2 sampleUV = v_texcoord0 + offset;

        float sampleValue = texture2D(s_tex_input, sampleUV).r;
        float sampleDepth = getLinearDepth(texture2D(s_tex_depth, sampleUV).r);

        // 1. Precise Gaussian Spatial Weight
        float distWeight = exp(-(i * i) / max(0.01, twoSigmaSq));
        
        // 2. Bilateral Range Weight
        // Using a smooth exponential falloff for depth
        float depthDiff = abs(centerDepth - sampleDepth);
        float depthWeight = exp(-depthDiff * sharpness);
        
        float finalWeight = distWeight * depthWeight;

        totalValue += sampleValue * finalWeight;
        totalWeight += finalWeight;
    }

    gl_FragColor = vec4(vec3_splat(totalValue / totalWeight), 1.0);
}