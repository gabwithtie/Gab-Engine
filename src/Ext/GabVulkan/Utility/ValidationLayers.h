#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>

namespace gbe::vulkan {
	class ValidationLayers {

    public:
        static const inline std::vector<const char*> validationLayerNames = {
            "VK_LAYER_KHRONOS_validation"
        };

		static void Check(std::vector<const char*>& allextensions) {
            allextensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            
            bool validationlayerssupported = true;

            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
            for (const char* layerName : validationLayerNames) {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    validationlayerssupported = false;
                }
            }
            if (!validationlayerssupported) {
                throw std::runtime_error("validation layers requested, but not available!");
            }
		}
	};
}