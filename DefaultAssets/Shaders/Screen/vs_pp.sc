$input a_position, a_texcoord0
$output v_texcoord0

#include "common.sh"

void main()
{
    // Pass the texture coordinates to the fragment shader
    v_texcoord0 = a_texcoord0;

    // The position is already in NDC (-1 to 1) from our C++ code
    gl_Position = vec4(a_position, 1.0);
}