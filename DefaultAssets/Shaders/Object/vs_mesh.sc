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
	// 1. Calculate the Model Matrix from instance data
	mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

	// 2. World Position (used for shadows and lighting)
	vec4 worldPos = mul(model, vec4(a_position, 1.0));
	v_pos = worldPos.xyz;
	v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;

	// 3. Clip Position (required for rasterization)
	gl_Position = mul(u_viewProj, worldPos);

	// 4. World Normal
	mat3 normalMatrix = (mat3)model;
	v_normal = normalize(mul(normalMatrix, a_normal));

	// 5. View Vector (World Space)
	vec3 camPos = u_invView[3].xyz; 
	v_view = camPos - worldPos.xyz;

    v_tangent = normalize(mul(normalMatrix, a_tangent.xyz));
	v_bitangent = cross(v_normal, v_tangent);
}
