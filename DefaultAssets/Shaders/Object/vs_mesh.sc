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

	// 5. View Vector (World Space)
	vec3 camPos = u_invView[3].xyz; 
	v_view = camPos - worldPos.xyz;

	// --- MANUAL NORMAL MATRIX (Adjugate) ---
    // Extract the basis vectors (columns) of the model matrix
    vec3 col0 = normalize(i_data0.xyz);
    vec3 col1 = normalize(i_data1.xyz);
    vec3 col2 = normalize(i_data2.xyz);

    // The cross products of the columns generate the Inverse-Transpose directions.
    // This correctly handles non-uniform scaling distortion.
    mat3 normalMatrix = mat3(
        cross(col1, col2), // New X-axis
        cross(col2, col0), // New Y-axis
        cross(col0, col1)  // New Z-axis
    );

    v_normal = normalize(mul(normalMatrix, a_normal));

    // Tangents are direction vectors, so they use the same matrix
    v_tangent = normalize(mul(normalMatrix, a_tangent.xyz));

    // BITANGENT: This is where the 'w' goes! 
    // This handles the UV mirroring (handedness)
    v_bitangent = normalize(cross(v_normal, v_tangent) * a_tangent.w);
}
