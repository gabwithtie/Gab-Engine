#pragma once

#include "../VulkanObject.h"
#include "CommandBufferSingle.h"

namespace gbe::vulkan {
    class Buffer : public VulkanObject<VkBuffer, Buffer> {
    private:
        VkDeviceMemory bufferMemory;
        unsigned int _size; //for debug
    protected:
    public:

        inline ~Buffer(){
            vkDestroyBuffer(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
            vkFreeMemory(VirtualDevice::GetActive()->GetData(), bufferMemory, nullptr);
        }

        VkDeviceMemory GetMemory() {
            return bufferMemory;
        }

        inline void RegisterDependencies() override {

        }

        inline Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
            _size = size;
            
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            CheckSuccess(vkCreateBuffer(VirtualDevice::GetActive()->GetData(), &bufferInfo, nullptr, &this->data));

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(VirtualDevice::GetActive()->GetData(), this->data, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = PhysicalDevice::GetActive()->FindMemoryType(memRequirements.memoryTypeBits, properties);

            CheckSuccess(vkAllocateMemory(VirtualDevice::GetActive()->GetData(), &allocInfo, nullptr, &bufferMemory));
            vkBindBufferMemory(VirtualDevice::GetActive()->GetData(), this->data, bufferMemory, 0);
        }

        inline static void CopyBuffer(Buffer* src, Buffer* dst, VkDeviceSize size) {
            CommandBufferSingle cmd;
            cmd.Begin();

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(cmd.GetData(), src->GetData(), dst->GetData(), 1, &copyRegion);

            cmd.End();
        }
    };
}