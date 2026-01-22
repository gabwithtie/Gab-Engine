$input v_texcoord0, v_viewDir

#include <bgfx_shader.sh>

// Uniforms for our gradient
vec4 u_skyColorTop = vec4(0.2f, 0.5f, 0.9f, 1.0f);    // Zenith color
vec4 u_skyColorBottom = vec4(0.7f, 0.8f, 1.0f, 1.0f); // Horizon color

void main() {
    // Normalize the view direction
    vec3 dir = normalize(v_viewDir);
    
    // Calculate 'up' factor based on Y component (-1 to 1 range converted to 0 to 1)
    // Using smoothstep creates a more natural atmospheric transition
    float up = smoothstep(-0.3, 0.1, dir.y);
    
    vec3 finalColor = mix(vec3(0.2f, 0.5f, 0.9f), vec3(0.7f, 0.8f, 1.0f), up);
    
    gl_FragColor = vec4(finalColor, 1.0);
}