#pragma once

#include "../VulkanObject.h"
#include "VirtualDevice.h"

#include "Image.h"

namespace gbe::vulkan {
	class ImageView : public VulkanObject<VkImageView> {

	public:
		inline void RegisterDependencies() override {

		}

        inline ~ImageView() {
            if (!initialized)
                return;

            vkDestroyImageView(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }

        inline ImageView() {

        }

        inline ImageView(Image& image, VkImageAspectFlags aspectflags)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image.GetData();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = image.Get_format();
            viewInfo.subresourceRange.aspectMask = aspectflags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            CheckSuccess(vkCreateImageView(VirtualDevice::GetActive()->GetData(), &viewInfo, nullptr, &this->data));
            initialized = true;
        }
	};
}