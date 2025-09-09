#pragma once

#include "../VulkanObject.h"

#include "CommandBufferSingle.h"
#include "Buffer.h"

namespace gbe::vulkan {
	class Image : public VulkanObject<VkImage> {
    private:
        VkDeviceMemory imageMemory;
        VkFormat format;
        VkImageTiling tiling;
        VkImageLayout layout;
        uint32_t width;
        uint32_t height;

	public:
		inline VkDeviceMemory Get_imageMemory() {
			return imageMemory;
		}
		inline VkFormat Get_format() {
			return format;
		}
		inline VkImageTiling Get_tiling() {
			return tiling;
		}
		inline VkImageLayout Get_layout() {
			return layout;
		}
		inline uint32_t Get_width() {
			return width;
		}
		inline uint32_t Get_height() {
			return height;
		}

		inline void RegisterDependencies() override {

		}
        inline ~Image() {
            if (!initialized)
                return;

            vkDestroyImage(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
            vkFreeMemory(VirtualDevice::GetActive()->GetData(), imageMemory, nullptr);
        }

        inline Image() {

        }

        inline Image(const VkImage& createdimage)
        {
            this->data = createdimage;
            initialized = true;
        }
        inline Image(uint32_t _width, uint32_t _height, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
        {
            width = _width;
            height = _height;
            format = _format;
            tiling = _tiling;
            layout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            CheckSuccess(vkCreateImage(VirtualDevice::GetActive()->GetData(), &imageInfo, nullptr, &this->data));

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(VirtualDevice::GetActive()->GetData(), this->data, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = PhysicalDevice::GetActive()->FindMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(VirtualDevice::GetActive()->GetData(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate image memory!");
            }

            CheckSuccess(vkBindImageMemory(VirtualDevice::GetActive()->GetData(), this->data, imageMemory, 0));
            initialized = true;
        }

        inline void transitionImageLayout(VkImageLayout newLayout)
        {
            CommandBufferSingle commandBuffer;
            commandBuffer.Begin();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.newLayout = newLayout;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = this->data;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }
            else {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else if (layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                commandBuffer.GetData(),
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            commandBuffer.End();

            layout = newLayout;
        }

        inline static void copyBufferToImage(Buffer& buffer, Image& image)
        {
            CommandBufferSingle commandBuffer;
            commandBuffer.Begin();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                image.width,
                image.height,
                1
            };

            vkCmdCopyBufferToImage(
                commandBuffer.GetData(),
                buffer.GetData(),
                image.GetData(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            commandBuffer.End();
        }

        inline static void copyImageToBuffer(Image image, Buffer buffer)
        {
            CommandBufferSingle commandBuffer;
            commandBuffer.Begin();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                image.width,
                image.height,
                1
            };

            image.transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            vkCmdCopyImageToBuffer(
                commandBuffer.GetData(),
                image.GetData(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                buffer.GetData(),
                1,
                &region
            );

            
            commandBuffer.End();
        }
    };
}