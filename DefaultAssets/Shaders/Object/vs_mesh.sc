$input a_position, a_normal, i_data0, i_data1, i_data2, i_data3
$output v_pos, v_view, v_normal, v_color0

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

	// 3. Clip Position (required for rasterization)
	gl_Position = mul(u_viewProj, worldPos);

	// 4. World Normal
	mat3 normalMatrix = (mat3)model;
	v_normal = normalize(mul(normalMatrix, a_normal));

	// 5. View Vector (World Space)
	vec3 camPos = u_invView[3].xyz; 
	v_view = camPos - worldPos.xyz;
}
