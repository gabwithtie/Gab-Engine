#pragma once

#include "../VulkanObject.h"

#include "RenderPass.h"

namespace gbe::vulkan {
    class Framebuffer : public VulkanObject<VkFramebuffer, Framebuffer> {
    protected:
    public:
        inline ~Framebuffer(){
            vkDestroyFramebuffer(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }
        inline void RegisterDependencies() override {

        }

        inline Framebuffer(uint32_t x, uint32_t y, RenderPass* renderpass, std::vector<VkImageView>& attachments) {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderpass->GetData();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = x;
            framebufferInfo.height = y;
            framebufferInfo.layers = 1;

            CheckSuccess(vkCreateFramebuffer(VirtualDevice::GetActive()->GetData(), &framebufferInfo, nullptr, &this->data));
        }
    };
}