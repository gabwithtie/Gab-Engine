#pragma once

#include "../VulkanObject.h"
#include "CommandPool.h"
#include "VirtualDevice.h"

namespace gbe::vulkan {
    class CommandBuffer : public VulkanObject<VkCommandBuffer, CommandBuffer> {
        VkCommandPool commandPool;
    protected:
    public:
        inline ~CommandBuffer(){
            vkFreeCommandBuffers(VirtualDevice::GetActive()->GetData(), commandPool, 1, &this->data);
        }
        inline void RegisterDependencies() override {

        }

        inline CommandBuffer(VkCommandPool commandPool) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            CheckSuccess(vkAllocateCommandBuffers(VirtualDevice::GetActive()->GetData(), &allocInfo, &this->data));
        }

        inline void Begin(CommandPool* pool = nullptr) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            CheckSuccess(vkBeginCommandBuffer(this->data, &beginInfo));
        }

        inline void End() {
            CheckSuccess(vkEndCommandBuffer(this->data));
        }
    };
}