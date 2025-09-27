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

        inline ImageView(Image* image, VkImageAspectFlags aspectflags)
        {
            this->image = image;

            image->SetAspectFlag(aspectflags);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image->GetData();
            if (image->Get_layercount() == 1) {
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.subresourceRange.layerCount = 1;
            }
            else {
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                viewInfo.subresourceRange.layerCount = image->Get_layercount();
            }
            viewInfo.format = image->Get_format();
            viewInfo.subresourceRange.aspectMask = aspectflags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;

            CheckSuccess(vkCreateImageView(VirtualDevice::GetActive()->GetData(), &viewInfo, nullptr, &this->data));
        }

        inline ImageView(Image* image, VkImageAspectFlags aspectflags, uint32_t i_index)
        {
            this->image = image;

            image->SetAspectFlag(aspectflags);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image->GetData();

            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.subresourceRange.layerCount = 1;

            viewInfo.format = image->Get_format();
            viewInfo.subresourceRange.aspectMask = aspectflags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;

            viewInfo.subresourceRange.baseArrayLayer = i_index;

            CheckSuccess(vkCreateImageView(VirtualDevice::GetActive()->GetData(), &viewInfo, nullptr, &this->data));
        }
    };
}