#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

namespace gbe::vulkan {
    class Surface : public VulkanObject<VkSurfaceKHR>, public VulkanObjectSingleton<Surface> {

    public:
        inline void RegisterDependencies() override {

        }

        inline Surface(VkSurfaceKHR surface) {
            this->data = surface;
            initialized = true;
        }
    };
}