$input v_texcoord0

#include "common.sh"

SAMPLER2D(tex_normal, 0); 
SAMPLER2D(tex_depth,  1);

// x: thickness, y: sensitivity threshold, z: screenWidth, w: screenHeight
uniform vec4 u_ssao_params;  
// x: near plane, y: far plane
uniform vec4 u_camera_params; 

float getLinearDepth(float _rawDepth)
{
    float n = u_camera_params.x;
    float f = u_camera_params.y;
    return (2.0 * n * f) / max(0.00001, (f + n - _rawDepth * (f - n)));
}

void main() {
    float rawDepth = texture2D(tex_depth, v_texcoord0).r;
    
    

    vec2 invScreen = 1.0 / max(u_camera_params.zw, vec2(1.0, 1.0));
    vec2 offset = u_ssao_params.xx * invScreen;

    // Center data
    float depthC = getLinearDepth(rawDepth);
    vec3 normalC = normalize(texture2D(tex_normal, v_texcoord0).xyz);

    // Neighbors (Left, Right, Top, Bottom)
    vec3 nL = normalize(texture2D(tex_normal, v_texcoord0 + vec2(-offset.x, 0.0)).xyz);
    vec3 nR = normalize(texture2D(tex_normal, v_texcoord0 + vec2( offset.x, 0.0)).xyz);
    vec3 nT = normalize(texture2D(tex_normal, v_texcoord0 + vec2(0.0,  offset.y)).xyz);
    vec3 nB = normalize(texture2D(tex_normal, v_texcoord0 + vec2(0.0, -offset.y)).xyz);

    float dL = getLinearDepth(texture2D(tex_depth, v_texcoord0 + vec2(-offset.x, 0.0)).r);
    float dR = getLinearDepth(texture2D(tex_depth, v_texcoord0 + vec2( offset.x, 0.0)).r);
    float dT = getLinearDepth(texture2D(tex_depth, v_texcoord0 + vec2(0.0,  offset.y)).r);
    float dB = getLinearDepth(texture2D(tex_depth, v_texcoord0 + vec2(0.0, -offset.y)).r);

    // 1. Grazing Angle Protection
    float NdotV = clamp(normalC.z, 0.0, 1.0);
    float threshold = u_ssao_params.y * (1.0 / max(NdotV, 0.1));

    // 2. Convex (Outer) Normal Logic
    // For an outer edge, the Left normal points Left (-X) and Right normal points Right (+X)
    // So (Right.x - Left.x) will be a positive value.
    float horizontalDelta = nR.x - nL.x;
    float verticalDelta   = nT.y - nB.y;
    
    float outerNormalEdge = abs(horizontalDelta) + abs(verticalDelta);
    outerNormalEdge = smoothstep(u_ssao_params.y, u_ssao_params.y * 2.0, outerNormalEdge);

    // 3. Depth "Ridge" Check
    // For an outer edge, the center pixel is CLOSER (smaller value) than the neighbors.
    // Neighbors - Center = Positive Value
    float depthRidge = abs(((dL + dR + dT + dB) * 0.25) - depthC);
    
    // Scale by depthC to keep sensitivity consistent with distance
    float outerDepthEdge = smoothstep(threshold, threshold * 2.0, max(0.0, depthRidge) / depthC);

    // 4. Combine and Filter
    // We want to avoid silhouettes (where the background is very far away)
    // If any neighbor is significantly further than the center, it's a silhouette/outer edge.
    // If you ONLY want convex corners on a single object, we add a cap:
    if (dL > depthC * 1.1 || dR > depthC * 1.1) {
        // This suppresses edges against the far background
        // outerDepthEdge *= 0.5; 
    }

    float edge = max(outerNormalEdge, outerDepthEdge);
    
    gl_FragColor = vec4(vec4_splat(clamp(edge, 0.0, 1.0)));
}