$input v_pos, v_view, v_normal, v_color0, v_texcoord0, v_tangent, v_bitangent
/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bgfx_shader.sh>

uniform vec4 id;

void main() {
    gl_FragColor = vec4(vec3(id), 1);
}