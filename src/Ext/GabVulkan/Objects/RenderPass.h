#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"
#include "Structures/AttachmentDictionary.h"

#include "VirtualDevice.h"

#include <array>

namespace gbe::vulkan {
    class RenderPass : public VulkanObject<VkRenderPass, RenderPass>, public VulkanObjectSingleton<RenderPass> {

    public:
        inline void RegisterDependencies() override {

        }

        inline ~RenderPass(){
            vkDestroyRenderPass(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }

        inline RenderPass(VkRenderPass existing) {
            this->data = existing;
        }
    };
}