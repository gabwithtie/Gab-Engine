#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include "Surface.h"
#include "FrameBuffer.h"
#include "RenderPass.h"

namespace gbe::vulkan {
	class SwapChain : public VulkanObject<VkSwapchainKHR>, public VulkanObjectSingleton<SwapChain> {

		std::vector<Image> swapChainImages;
		VkSurfaceFormatKHR chosenFormat;
		VkExtent2D swapchainExtent;
		std::vector<ImageView> swapChainImageViews;
		//SWAPCHAIN FRAMEBUFFERS
		std::vector<Framebuffer> swapChainFramebuffers;

	public:

		inline void RegisterDependencies() override {

		}

        inline ~SwapChain() {
            vkDestroySwapchainKHR(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
        }

        inline SwapChain() {

        }

		inline SwapChain(VkSurfaceFormatKHR _chosenFormat, VkPresentModeKHR chosenPresentMode, VkExtent2D _swapchainExtent, uint32_t imageCount) {
            
            chosenFormat = _chosenFormat;
            swapchainExtent = _swapchainExtent;

            VkSwapchainCreateInfoKHR swapchainInfo{};
            swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainInfo.surface = Surface::GetActive()->GetData();

            swapchainInfo.minImageCount = imageCount;
            swapchainInfo.imageFormat = chosenFormat.format;
            swapchainInfo.imageColorSpace = chosenFormat.colorSpace;
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
            swapchainInfo.presentMode = chosenPresentMode;
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
                swapChainImages.push_back(Image(vkimg));
            }

            //Image views
            swapChainImageViews.resize(swapChainImages.size());

            for (uint32_t i = 0; i < swapChainImages.size(); i++) {
                swapChainImageViews[i] = ImageView(swapChainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);
            }

            initialized = true;
		}

        inline void InitializeFramebuffers(ImageView depthImageView, RenderPass renderPass) {
            this->swapChainFramebuffers.resize(this->swapChainImageViews.size());
            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                std::array<VkImageView, 2> attachments = {
                    swapChainImageViews[i].GetData(),
                    depthImageView.GetData()
                };

                swapChainFramebuffers[i] = Framebuffer(swapchainExtent.width, swapchainExtent.height, renderPass, attachments);
            }
        }
	};
}