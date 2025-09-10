#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

namespace gbe::vulkan {
    class Surface : public VulkanObject<VkSurfaceKHR, Surface>, public VulkanObjectSingleton<Surface> {
        VkInstance owner;
    public:
        inline void RegisterDependencies() override {

        }

        inline ~Surface(){
            vkDestroySurfaceKHR(owner, this->data, nullptr);
        }

        inline Surface(VkSurfaceKHR surface, VkInstance _owner) {
            owner = _owner;
            this->data = surface;
        }
    };
}