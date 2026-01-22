$input v_worldPos

#include <bgfx_shader.sh>

uniform vec4 camera_pos;

// Helper to draw the grid lines
float grid(vec2 _p, float _res) {
    vec2 coord = _p * _res;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float nline = min(grid.x, grid.y);
    return 1.0 - min(nline, 1.0);
}

void main() {
    // 1. Calculate the ray from camera to world position
    vec3 u_viewPos = camera_pos;
    vec3 rayDir = normalize(v_worldPos - u_viewPos.xyz);
    
    // 2. Intersect ray with Y=0 plane
    // t = (plane_y - ray_start_y) / ray_dir_y
    float t = -u_viewPos.y / rayDir.y;
    
    // Do not render if the plane is behind the camera or too far
    if (t <= 0.0) discard;

    vec3 hitPos = u_viewPos.xyz + rayDir * t;
    vec2 uv = hitPos.xz;

    // 3. Create two grids (Large and Small) for a nice look
    float line1 = grid(uv, 1.0);  // 1 meter lines
    float line2 = grid(uv, 0.1);  // 10 meter lines
    
    // 4. Solve the Horizon Noise (Fade out based on distance)
    float opacity = smoothstep(1000.0, 200.0, t); // Fade out at 200-1000m
    
    // Mask out the "behind the horizon" area
    if (rayDir.y > -0.01) opacity = 0.0;

    vec3 color = mix(vec3(0.3), vec3(0.8), line2);
    float finalAlpha = max(line1 * 0.5, line2) * opacity;

    gl_FragColor = vec4(color, finalAlpha);

    // Project the world hit position back to screen space
    vec4 screenPos = mul(u_viewProj, vec4(hitPos, 1.0));
    
    // Calculate the 0-1 depth value
    float actualDepth = screenPos.z / screenPos.w;
    
    // Write it to the hardware depth buffer
    gl_FragDepth = actualDepth;
}