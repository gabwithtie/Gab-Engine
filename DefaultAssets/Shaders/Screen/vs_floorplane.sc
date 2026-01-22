$input a_position
$output v_worldPos

#include <bgfx_shader.sh>

void main() {
    // Standard full-screen triangle position
    gl_Position = vec4(a_position.xy, 1, 1.0);

    // Unproject to find the world-space ray
    vec4 unprojected = mul(u_invViewProj, gl_Position);
    vec3 worldPos = unprojected.xyz / unprojected.w;
    
    v_worldPos = worldPos;
}