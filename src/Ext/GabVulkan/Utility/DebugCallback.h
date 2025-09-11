#pragma once

#include <stdexcept>
#include <iostream>

#include <vulkan/vulkan.h>

namespace gbe::vulkan {
    static VKAPI_ATTR VkBool32 VKAPI_CALL UserDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "validation ERROR: " << pCallbackData->pMessage << std::endl;
            throw std::runtime_error("Vulkan validation layer error encountered: " + std::string(pCallbackData->pMessage));
        }
        else {
            std::cerr << "validation log: " << pCallbackData->pMessage << std::endl;
        }

        return VK_FALSE;
    }
}