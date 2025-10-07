#version 450

//TEXTURES

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 vertColor;
layout(location = 3) in vec3 fragT;
layout(location = 4) in vec3 fragB;
layout(location = 5) in vec3 fragN;

layout(location = 6) in float fraglight_nearclip;
layout(location = 7) in float fraglight_range;
layout(location = 8) in mat4 fraglight_projview;

layout(location = 0) out vec4 outColor;


void main() {

    if(fraglight_nearclip == 0){
        outColor = vec4(gl_FragCoord.z * 0.5 + 0.5, 0, 0,1);
    }
    else{
        float near = fraglight_nearclip;
        float far = fraglight_range;

        vec4 fragPosLightSpace = fraglight_projview * vec4(fragPos, 1.0);
		vec3 shadowcoord = fragPosLightSpace.xyz / fragPosLightSpace.w;
        shadowcoord = shadowcoord * 0.5 + 0.5;

        float linear_d = (2.0 * near * far) / (far + near - shadowcoord.z * (far - near));
        linear_d = linear_d / far; // normalize

        outColor = vec4(linear_d, 0, 0,1);
    }
}