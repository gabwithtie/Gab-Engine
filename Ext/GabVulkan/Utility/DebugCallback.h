#pragma once

#include <stdexcept>
#include <iostream>

#include <vulkan/vulkan.h>

namespace gbe::vulkan {
    class VulkanException : public std::exception {
    private:
        std::string message;

    public:
        // Constructor to set the error message
        inline VulkanException(const std::string msg){
            message = msg;
        }

        // Override the what() method to return the error message
        inline const char* what() const noexcept override {
            return message.c_str();
        }
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL UserDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            //std::cerr << "validation WARNING: " << pCallbackData->pMessage;
        }
        else if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "validation ERROR: " << pCallbackData->pMessage << std::endl;
            throw VulkanException("validation ERROR");
        }
        else
            std::cerr << "validation LOG: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
}