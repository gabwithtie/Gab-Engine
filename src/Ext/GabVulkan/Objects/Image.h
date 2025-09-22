#pragma once

#include "../VulkanObject.h"

#include "CommandBufferSingle.h"
#include "Buffer.h"

namespace gbe::vulkan {
	class Image : public VulkanObject<VkImage, Image> {
    private:
        VkDeviceMemory imageMemory;
        VkFormat format;
        VkImageTiling tiling;
        VkImageLayout layout;
        uint32_t width;
        uint32_t height;
        uint32_t layercount = 1;

        VkImageAspectFlags aspectflag;

	public:
        inline void SetAspectFlag(VkImageAspectFlags aspectflag) {
            this->aspectflag = aspectflag;
        }
        inline VkImageAspectFlags GetAspectFlag() {
            return aspectflag;
        }
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
        inline uint32_t Get_layercount()
        {
            return layercount;
        }

		inline void RegisterDependencies() override {

		}
        inline ~Image() {
            vkDestroyImage(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
            vkFreeMemory(VirtualDevice::GetActive()->GetData(), imageMemory, nullptr);
        }

        inline Image(
            const VkImage& createdimage,
            VkImageLayout _layout,
            VkFormat _format,
            uint32_t x, uint32_t y,
            uint32_t layercount = 1)
        {
            this->data = createdimage;

            this->layercount = layercount;
			layout = _layout;
			format = _format;
			width = x;
			height = y;
            tiling = VK_IMAGE_TILING_OPTIMAL;
        }
        inline Image(
            uint32_t _width, uint32_t _height, 
            VkFormat _format, 
            VkImageTiling _tiling, 
            VkImageUsageFlags usage, 
            VkMemoryPropertyFlags properties, 
            uint32_t layercount = 1)
        {
            width = _width;
            height = _height;
            format = _format;
            tiling = _tiling;
            this->layercount = layercount;
            layout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = layercount;
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
        }

        // A helper function to set access masks and pipeline stages based on the image layout
        static void setBarrierInfo(
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask,
            VkPipelineStageFlags& srcStageMask, VkPipelineStageFlags& dstStageMask) {

            // Default to a safe, but less optimal, full synchronization
            srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            // Add more specific layout transitions as needed for your application.
            // The general fall-through case handles all other transitions but might be less performant.
        }

        inline void transitionImageLayout(VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
        {
            CommandBufferSingle commandBuffer;
            commandBuffer.Begin();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.newLayout = newLayout;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = this->data;
            barrier.subresourceRange.aspectMask = aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

        inline static void copyBufferToImage(Buffer* buffer, Image* image)
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
                image->width,
                image->height,
                1
            };

            vkCmdCopyBufferToImage(
                commandBuffer.GetData(),
                buffer->GetData(),
                image->GetData(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            commandBuffer.End();
        }

        inline static void copyImageToBuffer(Image* image, Buffer* buffer)
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
                image->width,
                image->height,
                1
            };

            image->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            vkCmdCopyImageToBuffer(
                commandBuffer.GetData(),
                image->GetData(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                buffer->GetData(),
                1,
                &region
            );

            
            commandBuffer.End();
        }

        static void copyImageToImage(Image* srcImage, Image* dstImage, VkImageLayout dstFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED) {
            CommandBufferSingle commandBuffer;
            commandBuffer.Begin();

            // Transition source image to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            VkAccessFlags srcAccessSrc, srcAccessDst;
            VkPipelineStageFlags srcStageSrc, srcStageDst;
            setBarrierInfo(srcImage->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcAccessSrc, srcAccessDst, srcStageSrc, srcStageDst);

            VkImageMemoryBarrier srcBarrier = {};
            srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            srcBarrier.oldLayout = srcImage->layout;
            srcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            srcBarrier.srcAccessMask = srcAccessSrc;
            srcBarrier.dstAccessMask = srcAccessDst;
            srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            srcBarrier.image = srcImage->GetData();
            srcBarrier.subresourceRange.aspectMask = srcImage->GetAspectFlag();
            srcBarrier.subresourceRange.baseMipLevel = 0;
            srcBarrier.subresourceRange.levelCount = 1;
            srcBarrier.subresourceRange.baseArrayLayer = 0;
            srcBarrier.subresourceRange.layerCount = 1;

            // Transition destination image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            VkAccessFlags dstAccessSrc, dstAccessDst;
            VkPipelineStageFlags dstStageSrc, dstStageDst;
            setBarrierInfo(dstImage->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstAccessSrc, dstAccessDst, dstStageSrc, dstStageDst);

            VkImageMemoryBarrier dstBarrier = {};
            dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            dstBarrier.oldLayout = dstImage->layout;
            dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            dstBarrier.srcAccessMask = dstAccessSrc;
            dstBarrier.dstAccessMask = dstAccessDst;
            dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            dstBarrier.image = dstImage->GetData();
            dstBarrier.subresourceRange.aspectMask = dstImage->GetAspectFlag();
            dstBarrier.subresourceRange.baseMipLevel = 0;
            dstBarrier.subresourceRange.levelCount = 1;
            dstBarrier.subresourceRange.baseArrayLayer = 0;
            dstBarrier.subresourceRange.layerCount = 1;

            // Place both barriers in a single vkCmdPipelineBarrier call
            vkCmdPipelineBarrier(
                commandBuffer.GetData(),
                srcStageSrc | dstStageSrc, // Combined source stages
                srcStageDst | dstStageDst, // Combined destination stages
                0, // dependencyFlags
                0, nullptr,
                0, nullptr,
                2, new VkImageMemoryBarrier[2]{ srcBarrier, dstBarrier }
            );

            // Define the copy region
            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = srcImage->GetAspectFlag();
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = { 0, 0, 0 };
            copyRegion.dstSubresource.aspectMask = dstImage->GetAspectFlag();
            copyRegion.dstSubresource.mipLevel = 0;
            copyRegion.dstSubresource.baseArrayLayer = 0;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = { 0, 0, 0 };
            copyRegion.extent = { srcImage->width, srcImage->height, 1 };

            // Execute the copy command
            vkCmdCopyImage(
                commandBuffer.GetData(),
                srcImage->GetData(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                dstImage->GetData(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, // regionCount
                &copyRegion
            );

            // Transition both images to their final layouts
            setBarrierInfo(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcImage->Get_layout(), srcAccessSrc, srcAccessDst, srcStageSrc, srcStageDst);
            srcBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            srcBarrier.newLayout = srcImage->Get_layout();
            srcBarrier.srcAccessMask = srcAccessSrc;
            srcBarrier.dstAccessMask = srcAccessDst;
            srcBarrier.subresourceRange.aspectMask = srcImage->GetAspectFlag();

            if (dstFinalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                dstFinalLayout = dstImage->layout;

            if (dstFinalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                throw new std::runtime_error("Layout still undefined.");

            setBarrierInfo(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstFinalLayout, dstAccessSrc, dstAccessDst, dstStageSrc, dstStageDst);
            dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            dstBarrier.newLayout = dstFinalLayout;
            dstBarrier.srcAccessMask = dstAccessSrc;
            dstBarrier.dstAccessMask = dstAccessDst;
            dstBarrier.subresourceRange.aspectMask = dstImage->GetAspectFlag();

            vkCmdPipelineBarrier(
                commandBuffer.GetData(),
                srcStageSrc | dstStageSrc, // Combined source stages
                srcStageDst | dstStageDst, // Combined destination stages
                0, // dependencyFlags
                0, nullptr,
                0, nullptr,
                2, new VkImageMemoryBarrier[2]{ srcBarrier, dstBarrier }
            );


            commandBuffer.End();
        }
    };
}