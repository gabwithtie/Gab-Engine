#pragma once

#include "Math/gbe_math.h"

namespace gbe::vulkan {

    // The data we will send to the GPU for each light.
    // The layout must match the shader's uniform buffer.
    struct LightData {
        enum LightType {
            DIRECTIONAL = 0,
            SPOT = 1,
            POINT = 2
        };

        alignas(16) Vector3 position;       // For point/spot lights
        alignas(16) Vector3 direction;      // For directional/spot lights
        alignas(16) Vector3 color;
        LightType type;
    };

    // Data needed specifically for shadow casting lights on the GPU
    struct ShadowCastingData {
        alignas(16) Matrix4 lightSpaceMatrix; // Transforms world space to light's clip space
    };
}
