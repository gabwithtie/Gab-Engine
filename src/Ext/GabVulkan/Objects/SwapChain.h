#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include "Surface.h"
#include "FrameBuffer.h"
#include "RenderPass.h"
#include "PhysicalDevice.h"

#include "Structures/AttachmentReferencePasser.h"

namespace gbe::vulkan {
	class SwapChain : public VulkanObject<VkSwapchainKHR, SwapChain>, public VulkanObjectSingleton<SwapChain> {

		VkExtent2D swapchainExtent;

        //================DYNAMICALLY ALLOCATED=====================// 
		std::vector<Image*> swapChainImages;
		std::vector<ImageView*> swapChainImageViews;
		std::vector<Framebuffer*> swapChainFramebuffers;

	public:

		inline void RegisterDependencies() override {

		}

        inline Image* GetImage(int index) {
            return swapChainImages[index];
        }

        inline Framebuffer* GetFramebuffer(int index) {
            return swapChainFramebuffers[index];
        }

        inline VkExtent2D& GetExtent() {
            return swapchainExtent;
        }

        inline unsigned int GetImageCount() {
            return swapChainImages.size();
        }

        inline ~SwapChain() {
            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                delete swapChainImageViews[i];
            }
            for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
                delete swapChainFramebuffers[i];
            }

            vkDestroySwapchainKHR(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }

		inline SwapChain(VkExtent2D _swapchainExtent, uint32_t imageCount) {
            VkSwapchainCreateInfoKHR swapchainInfo{};
            swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainInfo.surface = Surface::GetActive()->GetData();

            swapchainInfo.minImageCount = imageCount;
            swapchainInfo.imageFormat = PhysicalDevice::GetActive()->Get_swapchainFormat().format;
            swapchainInfo.imageColorSpace = PhysicalDevice::GetActive()->Get_swapchainFormat().colorSpace;
            swapchainInfo.imageExtent = swapchainExtent;
            swapchainInfo.imageArrayLayers = 1;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            uint32_t queueFamilyIndices[] = { PhysicalDevice::GetActive()->Get_graphicsQueueIndex(), PhysicalDevice::GetActive()->Get_presentQueueIndex() };

            if (PhysicalDevice::GetActive()->Get_graphicsQueueIndex() != PhysicalDevice::GetActive()->Get_presentQueueIndex()) {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainInfo.queueFamilyIndexCount = 2;
                swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapchainInfo.queueFamilyIndexCount = 0; // Optional
                swapchainInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            swapchainInfo.preTransform = PhysicalDevice::GetActive()->Get_capabilities().currentTransform;
            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainInfo.presentMode = PhysicalDevice::GetActive()->Get_swapchainPresentMode();
            swapchainInfo.clipped = VK_TRUE;
            swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(VirtualDevice::GetActive()->GetData(), &swapchainInfo, nullptr, &this->data) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }

            //Retrieving the swap chain images
            vkGetSwapchainImagesKHR(VirtualDevice::GetActive()->GetData(), this->data, &imageCount, nullptr);
            std::vector<VkImage> swapchain_vkimages(imageCount);
            vkGetSwapchainImagesKHR(VirtualDevice::GetActive()->GetData(), this->data, &imageCount, swapchain_vkimages.data());

            for (auto& vkimg : swapchain_vkimages)
            {
                swapChainImages.push_back(new Image(vkimg, VK_IMAGE_LAYOUT_UNDEFINED, PhysicalDevice::GetActive()->Get_swapchainFormat().format, swapchainExtent.width, swapchainExtent.height));
            }

            //Image views
            swapChainImageViews.resize(swapChainImages.size());

            for (uint32_t i = 0; i < swapChainImages.size(); i++) {
                swapChainImageViews[i] = new ImageView(swapChainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);
            }
		}

        inline void InitializeFramebuffers(AttachmentReferencePasser& attachments, RenderPass* renderPass) {
            this->swapChainFramebuffers.resize(this->swapChainImageViews.size());
            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                attachments.PassView("color", swapChainImageViews[i]->GetData());
                swapChainFramebuffers[i] = new Framebuffer(swapchainExtent.width, swapchainExtent.height, renderPass, attachments);
            }
        }
    };
}