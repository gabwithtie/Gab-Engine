$input a_position, a_texcoord0
$output v_texcoord0, v_viewDir

#include "common.sh"

void main()
{
    // Pass the texture coordinates to the fragment shader
    v_texcoord0 = a_texcoord0;

    // The position is already in NDC (-1 to 1) from our C++ code
    gl_Position = vec4(a_position, 1.0);
    gl_Position.z = gl_Position.w;

    
    // Use built-in or custom inverse uniforms
    // u_invProj: Inverse of the Projection matrix
    // u_invView: Inverse of the View matrix (Rotation part only)

    vec4 viewPos = mul(u_invProj, vec4(a_position, 1.0));
    viewPos /= viewPos.w; // Perspective divide to get View Space coordinate

    // Multiply by inverse View Rotation to get World Space direction
    // We set w to 0.0 to ignore any translation data
    v_viewDir = mul(u_invView, vec4(viewPos.xyz, 0.0)).xyz;
}