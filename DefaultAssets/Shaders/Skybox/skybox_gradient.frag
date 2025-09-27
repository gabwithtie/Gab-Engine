#version 450

//TEXTURES

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 vertColor;
layout(location = 3) in vec3 fragT;
layout(location = 4) in vec3 fragB;
layout(location = 5) in vec3 fragN;
layout(location = 6) in vec3 camera_pos;

layout(location = 0) out vec4 outColor;

void main() {
    float atmosphereThickness = 0.9;
    vec3 horizonColor = vec3(0.6, 0.8, 1);
    vec3 skyColor = vec3(0.35, 0.65, 1);
    vec3 groundColor = vec3(0.4, 0.4, 0.4);

    vec3 viewDir = normalize(fragPos);

    // 2. Extract the vertical component (Y-axis) for the gradient factor.
    float factor = viewDir.y;

    // 3. Unity-like factor mapping (using pow for a non-linear falloff)
    float blendFactor = smoothstep(0.0, 1.0 - atmosphereThickness, factor);

    // 4. Blend the colors based on the vertical factor.
    vec3 finalColor;
    if (factor >= 0.0) {
        finalColor = mix(horizonColor, skyColor, blendFactor);
    } else {
        // Below the horizon: Blend between HorizonColor and GroundColor
        // We use the absolute value of the factor for the ground to make the mix work:
        // abs(factor) goes from 0 at horizon to 1 straight down.
        float groundBlend = smoothstep(0.0, 1.0, abs(factor) * (1.0 - atmosphereThickness));
        finalColor = mix(horizonColor, groundColor, groundBlend);
    }

    // 5. Output the color
    outColor = vec4(finalColor, 1);
}