#pragma once

#include "../VulkanObject.h"

namespace gbe::vulkan {
	class ImageView : public VulkanObject<VkImageView> {

	public:
		inline void RegisterDependencies() override {

		}

        void gbe::RenderPipeline::createImageView(VkImageView& imageview, VkImage image, VkFormat format, VkImageAspectFlags aspectflags)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectflags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(Instance->vkdevice, &viewInfo, nullptr, &imageview) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
	};
}