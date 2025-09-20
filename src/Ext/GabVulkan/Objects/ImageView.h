#pragma once

#include "../VulkanObject.h"
#include "VirtualDevice.h"

#include "Image.h"

namespace gbe::vulkan {
	class ImageView : public VulkanObject<VkImageView, ImageView> {
        Image* image;
	public:
        inline Image* Get_image() {
            return image;
        }

		inline void RegisterDependencies() override {

		}

        inline ~ImageView() {
            vkDestroyImageView(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }

        inline ImageView(Image* image, VkImageAspectFlags aspectflags, VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D, uint32_t i_index = 0)
        {
            this->image = image;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image->GetData();
            viewInfo.viewType = viewtype;
            viewInfo.format = image->Get_format();
            viewInfo.subresourceRange.aspectMask = aspectflags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = i_index;

            if (viewtype != VK_IMAGE_VIEW_TYPE_2D)
                viewInfo.subresourceRange.layerCount = image->Get_layercount();
            else
                viewInfo.subresourceRange.layerCount = 1;


            CheckSuccess(vkCreateImageView(VirtualDevice::GetActive()->GetData(), &viewInfo, nullptr, &this->data));
        }
	};
}