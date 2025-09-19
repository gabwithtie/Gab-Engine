#pragma once

#include "../VulkanObject.h"

#include "RenderPass.h"
#include "Structures/AttachmentReferencePasser.h"

namespace gbe::vulkan {
    class Framebuffer : public VulkanObject<VkFramebuffer, Framebuffer> {
    protected:
    public:
        inline ~Framebuffer(){
            vkDestroyFramebuffer(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }
        inline void RegisterDependencies() override {

        }

        inline Framebuffer(uint32_t x, uint32_t y, RenderPass* renderpass, AttachmentReferencePasser attachmentpasser) {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderpass->GetData();
            framebufferInfo.attachmentCount = attachmentpasser.GetSize();
            framebufferInfo.pAttachments = attachmentpasser.TryGetPasserPtr();
            framebufferInfo.width = x;
            framebufferInfo.height = y;
            framebufferInfo.layers = 1;

            CheckSuccess(vkCreateFramebuffer(VirtualDevice::GetActive()->GetData(), &framebufferInfo, nullptr, &this->data));
        }
    };
}