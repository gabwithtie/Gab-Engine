#pragma once

#include <vulkan/vulkan.h>

#include "../Objects/Image.h"

namespace gbe::vulkan {
	class MemoryBarrier {
	public:
		static void Insert(VkCommandBuffer cmdbuffer, Image* image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            imageMemoryBarrier.srcAccessMask = srcAccessMask;
            imageMemoryBarrier.dstAccessMask = dstAccessMask;
            imageMemoryBarrier.oldLayout = image->Get_layout();
            imageMemoryBarrier.newLayout = newImageLayout;
            imageMemoryBarrier.image = image->GetData();
            imageMemoryBarrier.subresourceRange = subresourceRange;

            vkCmdPipelineBarrier(
                cmdbuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }
	};
}