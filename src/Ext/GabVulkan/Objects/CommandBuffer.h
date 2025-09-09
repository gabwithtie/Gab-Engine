#pragma once

#include "../VulkanObject.h"
#include "CommandPool.h"
#include "VirtualDevice.h"

namespace gbe::vulkan {
    class CommandBufferSingle : public VulkanObject<VkCommandBuffer> {

    public:
        inline void RegisterDependencies() override {

        }

        inline void Begin() {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = CommandPool::Active()->data;
            allocInfo.commandBufferCount = 1;

            vkAllocateCommandBuffers(VirtualDevice::Active()->data, &allocInfo, &(this->data));

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
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(Instance->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(Instance->graphicsQueue);

            vkFreeCommandBuffers(Instance->vkdevice, Instance->commandPool, 1, &commandBuffer);
        }
    };
}