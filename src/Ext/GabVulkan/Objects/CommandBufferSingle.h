#pragma once

#include "../VulkanObject.h"
#include "CommandPool.h"
#include "VirtualDevice.h"

namespace gbe::vulkan {
    class CommandBufferSingle : public VulkanObject<VkCommandBuffer> {
        VkCommandPool commandPool;

    public:
        inline void RegisterDependencies() override {

        }

        inline void Begin(CommandPool* pool = nullptr) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            if (pool == nullptr)
                allocInfo.commandPool = CommandPool::GetActive()->GetData();
            else
                allocInfo.commandPool = pool->GetData();
            allocInfo.commandBufferCount = 1;

            vkAllocateCommandBuffers(VirtualDevice::GetActive()->GetData(), &allocInfo, &(this->data));

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(this->data, &beginInfo);
        }

        inline void End() {
            vkEndCommandBuffer(this->data);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &this->data;

            vkQueueSubmit(VirtualDevice::GetActive()->Get_graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(VirtualDevice::GetActive()->Get_graphicsQueue());

            vkFreeCommandBuffers(VirtualDevice::GetActive()->GetData(), this->commandPool, 1, &this->data);
            initialized = true;
        }
    };
}