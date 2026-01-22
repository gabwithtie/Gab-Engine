$input v_texcoord0

#include "common.sh"

SAMPLER2D(tex_buffer_a, 0); 
SAMPLER2D(tex_buffer_b, 1);

uniform vec4 u_blend_params;
#define u_blend_type u_blend_params.x

void main() {
    vec4 a = texture2D(tex_buffer_a, v_texcoord0).rgba;
    vec4 b = texture2D(tex_buffer_b, v_texcoord0).rgba;

    if(abs(u_blend_type) < 0.5) { // alpha blend
        gl_FragColor = vec4(mix(a, b, b.a).xyz, 1);
        return;
    }
}