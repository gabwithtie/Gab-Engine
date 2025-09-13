#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// G-Buffer inputs
layout(set = 0, binding = 0) uniform sampler2D positionBuffer;
layout(set = 0, binding = 1) uniform sampler2D normalBuffer;
layout(set = 0, binding = 2) uniform sampler2D albedoSpecBuffer;
layout(set = 0, binding = 3) uniform sampler2D shadowMap;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0: Dir, 1: Point
};

layout(set = 0, binding = 4) uniform LightBlock {
    Light lights[16]; // Max 16 lights
    int lightCount;
} lightBlock;

layout(set = 0, binding = 5) uniform ShadowBlock {
    mat4 lightSpaceMatrix;
} shadowBlock;


layout(set = 0, binding = 6) uniform CameraBlock {
    vec3 viewPos;
} cameraBlock;

float calculateShadow(vec3 worldPos) {
    vec4 fragPosLightSpace = shadowBlock.lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Transform to [0,1] range

    // Don't cast shadows on fragments outside the light's view
    if (projCoords.z > 1.0) return 1.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    // PCF for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - 0.005 > pcfDepth ? 0.0 : 1.0;        
        }
    }
    return shadow / 9.0;
}

void main() {
    vec3 worldPos = texture(positionBuffer, fragTexCoord).rgb;
    vec3 normal = normalize(texture(normalBuffer, fragTexCoord).rgb);
    vec4 albedoSpec = texture(albedoSpecBuffer, fragTexCoord);
    vec3 albedo = albedoSpec.rgb;
    float specularStrength = albedoSpec.a;

    vec3 ambient = 0.1 * albedo;
    vec3 lighting = vec3(0.0);

    for(int i = 0; i < lightBlock.lightCount; ++i) {
        Light light = lightBlock.lights[i];
        
        // Diffuse
        vec3 lightDir = normalize(-light.direction);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * light.color * light.intensity;

        // Specular
        vec3 viewDir = normalize(cameraBlock.viewPos - worldPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * light.color * light.intensity;
        
        float shadowFactor = calculateShadow(worldPos);
        lighting += (diffuse + specular) * shadowFactor;
    }

    outColor = vec4(ambient + lighting, 1.0) * vec4(albedo, 1.0);
}
