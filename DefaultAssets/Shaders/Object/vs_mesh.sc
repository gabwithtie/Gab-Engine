$input a_position, a_normal, a_tangent, a_color0, a_texcoord0, i_data0, i_data1, i_data2, i_data3
$output v_pos, v_view, v_normal, v_color0, v_texcoord0, v_tangent, v_bitangent

/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "common.sh"
#include <bgfx_shader.sh>

void main()
{
	//BGFX==========================
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

	vec3 wpos = mul(model, vec4(a_position, 1.0) ).xyz;
	gl_Position = mul(u_viewProj, vec4(wpos, 1.0) );

	vec3 normal = a_normal;
	vec3 wnormal = mul(model, vec4(normal.xyz, 0.0) ).xyz;

	vec4 tangent = a_tangent;
	vec3 wtangent = mul(model, vec4(tangent.xyz, 0.0) ).xyz;

	v_normal = wnormal;
	v_tangent = wtangent;
	v_bitangent = cross(v_normal, v_tangent) * tangent.w;

	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	v_pos = wpos;

	vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
	v_view = mul(weyepos - wpos, tbn);

	v_texcoord0 = a_texcoord0;
}
