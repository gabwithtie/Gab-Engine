$input v_pos, v_view, v_normal, v_color0, v_texcoord0, v_tangent, v_bitangent

#include "common.sh"

void main()
{
    // Normalize the interpolated normal
    vec3 normal = normalize(v_normal);

    // Write to gl_FragData[0] (this maps to m_gbufferNormal in your C++ code)
    // We store it as a vec4; the alpha can be 1.0 or used for material properties
    gl_FragData[0] = vec4(normal, 1.0);
    
    // Note: The hardware depth (m_gbufferDepth) is updated automatically 
    // by the GPU's depth-test hardware.
}