#pragma once

#include "Math/gbe_math.h"

namespace gbe::vulkan {

    // The data we will send to the GPU for each light.
    // The layout must match the shader's uniform buffer.
    struct LightData {
        alignas(16) Vector3 position;       // For point/spot lights
        alignas(16) Vector3 direction;      // For directional/spot lights
        alignas(16) Vector3 color;
        float intensity;
        int type; // 0: Directional, 1: Point
        // Other params like radius, cone angle etc. can go here
    };

    // Data needed specifically for shadow casting lights on the GPU
    struct ShadowCastingData {
        alignas(16) Matrix4 lightSpaceMatrix; // Transforms world space to light's clip space
    };
}
