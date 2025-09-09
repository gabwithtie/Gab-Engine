#pragma once

#include "../VulkanObject.h"
#include "CommandBuffer.h"

namespace gbe::vulkan {
	class Buffer : public VulkanObject<VkBuffer> {
	private:
		VkDeviceMemory bufferMemory;
	public:
		VkDeviceMemory GetMemory() {
			return bufferMemory;
		}

		inline void RegisterDependencies() override {

		}

		inline static Buffer Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            CheckSuccess(vkCreateBuffer(Instance->vkdevice, &bufferInfo, nullptr, &buffer));

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(Instance->vkdevice, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = Instance->findMemoryType(memRequirements.memoryTypeBits, properties);

            CheckSuccess(vkAllocateMemory(Instance->vkdevice, &allocInfo, nullptr, &bufferMemory));
            vkBindBufferMemory(Instance->vkdevice, buffer, bufferMemory, 0);
		}

        inline static void CopyBuffer(Buffer src, Buffer dst) {
			CommandBufferSingle cmd;
			cmd.Begin();

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(cmd.GetData(), src.GetData(), dst.GetData(), 1, &copyRegion);

			cmd.End();
        }
	}
};