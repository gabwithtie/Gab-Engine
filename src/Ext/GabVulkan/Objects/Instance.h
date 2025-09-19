#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include "../Utility/DebugCallback.h"
#include "../Utility/ValidationLayers.h"
#include "../Utility/DebugObjectName.h"

#include "../Components/Renderer.h"

#include <algorithm>
#include <array>
#include <vector>
#include <stack>
#include <queue>
#include <list>

#include "Image.h"
#include "ImageView.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "PhysicalDevice.h"
#include "VirtualDevice.h"
#include "CommandBuffer.h"
#include "Structures/FrameSyncronizationObject.h"


namespace gbe::vulkan {
    class Instance : public VulkanObject<VkInstance, Instance>, public VulkanObjectSingleton<Instance> {
    
        //INSTANCE INFO
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        bool enableValidationLayers = false;

        int &x;
        int &y;
        int MAX_FRAMES_IN_FLIGHT = 2;

        //STATES
        uint32_t currentFrame = 0;
        uint32_t currentSwapchainImage = 0;
        
        //==============DYNAMICALLY ALLOCATED============
        Surface* surface;
        PhysicalDevice* physicalDevice;
        VirtualDevice* virtualDevice;
        vulkan::SwapChain* swapchain; //remember that images are arbitrary
        RenderPass* renderPass;
        Image* depthImage;
        ImageView* depthImageView;
        CommandPool* commandPool;
        std::vector<CommandBuffer*> commandBuffers; //buffers per frame
        std::vector<FrameSyncronizationObject*> frameSynchronizationObjects; //sync objects per frame

		Renderer* customRenderer = nullptr;

        //==============DELETION QUEUES================//
		std::vector<Buffer*> bufferDeletionQueue;

    public:
        inline void RegisterDependencies() override {

        }

        inline const uint32_t Get_maxFrames() const {
            return MAX_FRAMES_IN_FLIGHT;
        }

        inline CommandBuffer* GetCurrentCommandBuffer() {
            return commandBuffers[currentFrame];
        }

        inline uint32_t GetCurrentFrameIndex() {
            return currentFrame;
        }

        inline uint32_t GetCurrentSwapchainImage() {
            return currentSwapchainImage;
        }

        inline Surface* GetSurface() {
            return surface;
        }

        inline ImageView* GetDepthImageView() {
            return depthImageView;
		}

        inline void SetCustomRenderer(Renderer* renderer) {
			this->customRenderer = renderer;

            this->RefreshPipelineObjects();
        }
        inline void QueueBufferDeletion(Buffer* buffer) {
			this->bufferDeletionQueue.push_back(buffer);
        }

        inline Instance(int &_x, int &_y, std::vector<const char*>& allextensions, bool _enableValidationLayers):
            x(_x),
            y(_y)
        {
            enableValidationLayers = _enableValidationLayers;

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "AnitoPCG";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "GabEngine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo instInfo{};
            instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instInfo.pApplicationInfo = &appInfo;
            instInfo.pNext = nullptr;
            instInfo.flags = 0;
            instInfo.ppEnabledLayerNames = nullptr;
            instInfo.enabledExtensionCount = static_cast<uint32_t>(allextensions.size());;
            instInfo.ppEnabledExtensionNames = allextensions.data();

            //DEBUG
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};

            if (enableValidationLayers) {
                instInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers::validationLayerNames.size());
                instInfo.ppEnabledLayerNames = ValidationLayers::validationLayerNames.data();

                debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debug_messenger_create_info.pfnUserCallback = UserDebugCallback;
                debug_messenger_create_info.pUserData = nullptr; // Optional

                instInfo.pNext = &debug_messenger_create_info;

                VkValidationFeatureEnableEXT enables[] = { 
                    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
                    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
                };
                VkValidationFeaturesEXT features{};
                features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                features.enabledValidationFeatureCount = 2;
                features.pEnabledValidationFeatures = enables;
				debug_messenger_create_info.pNext = &features;
            }
            else {
                instInfo.enabledLayerCount = 0;
                instInfo.pNext = nullptr;
            }

            //INSTANCE
            CheckSuccess(vkCreateInstance(&instInfo, nullptr, &this->data));

            if (enableValidationLayers) {
                vulkan::DebugObjectName::Init((PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(this->data, "vkSetDebugUtilsObjectNameEXT"));
                auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->data, "vkCreateDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(this->data, &debug_messenger_create_info, nullptr, &debugMessenger);
                }
                else {
                    throw std::runtime_error("failed to set up debug messenger!");
                }
            }
        }

        inline void Init(Surface* _surface) {
            //===================SURFACE SET UP===================//
            
            surface = _surface;
            Surface::SetActive(surface);
            //reconstruct because surface is expected to be constructed using dereferencing.

            //===================DEVICE SET UP===================//
            const std::vector<const char*> deviceExtensionNames = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
            };

            bool founddevice = false;
            uint32_t physicalDeviceCount;
            vkEnumeratePhysicalDevices(this->data, &physicalDeviceCount, nullptr);
            if (physicalDeviceCount == 0) {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(this->data, &physicalDeviceCount, physicalDevices.data());
            std::vector<PhysicalDevice*> gbephysicalDevices;
            for (const auto pd : physicalDevices)
            {
                gbephysicalDevices.push_back(new PhysicalDevice(pd));
            }

            //TEST DEVICE SUITABLE
            std::set<std::string> requiredExtensions(deviceExtensionNames.begin(), deviceExtensionNames.end());

            for (size_t i = 0; i < physicalDeviceCount; i++)
            {
                if (gbephysicalDevices[i]->IsCompatible(requiredExtensions) && gbephysicalDevices[i]->Get_Features().samplerAnisotropy) {
                    physicalDevice = gbephysicalDevices[i];
                    PhysicalDevice::SetActive(physicalDevice);
                    founddevice = true;
                }
                else {
                    delete gbephysicalDevices[i];
                }
            }

            if (founddevice == false) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }

            virtualDevice = new VirtualDevice(PhysicalDevice::GetActive(), deviceExtensionNames);
            VirtualDevice::SetActive(virtualDevice);

            //======================OBJECT SETUP========================
            this->InitializePipelineObjects();

            //======================CommandPool SETUP========================
            commandPool = new CommandPool();
            CommandPool::SetActive(commandPool);

            commandBuffers.resize(this->MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < this->MAX_FRAMES_IN_FLIGHT; i++)
            {
                commandBuffers[i] = new CommandBuffer(commandPool->GetData());
            }

            //======================DISPLAY SETUP========================
            this->InitializePipelineAttachments();
            this->swapchain->InitializeFramebuffers({ depthImageView->GetData() }, renderPass);

            if (customRenderer != nullptr) {
                
            }

            //======================SYNCHRONIZATION SETUP========================
            this->frameSynchronizationObjects.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                frameSynchronizationObjects[i] = new FrameSyncronizationObject(this->swapchain->GetImageCount());
            }
        }

        inline void InitializePipelineObjects() {
            PhysicalDevice::GetActive()->Refresh();

            //==============SWAPCHAIN================
            //format selection
            VkSurfaceFormatKHR chosenFormat;

            for (const auto& availableFormat : PhysicalDevice::GetActive()->Get_formats()) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    chosenFormat = availableFormat;
                    break;
                }
            }
            //presentation mode selection
            VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

            for (const auto& availablePresentMode : PhysicalDevice::GetActive()->Get_presentModes()) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    chosenPresentMode = availablePresentMode;
                }
            }

            //swapchain extent selection
            VkExtent2D swapchainExtent = {
                static_cast<uint32_t>(this->x),
                static_cast<uint32_t>(this->y)
            };

            swapchainExtent.width = std::clamp(swapchainExtent.width, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.width, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.width);
            swapchainExtent.height = std::clamp(swapchainExtent.height, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.height, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.height);

            //Image count determination
            uint32_t imageCount = PhysicalDevice::GetActive()->Get_capabilities().minImageCount + 1;
            if (PhysicalDevice::GetActive()->Get_capabilities().maxImageCount > 0 && imageCount > PhysicalDevice::GetActive()->Get_capabilities().maxImageCount) {
                imageCount = PhysicalDevice::GetActive()->Get_capabilities().maxImageCount;
            }

            swapchain = new SwapChain(chosenFormat, chosenPresentMode, swapchainExtent, imageCount);
            SwapChain::SetActive(swapchain);

            //==============RENDERPASS================
            if (this->customRenderer != nullptr)
                renderPass = this->customRenderer->CreateRenderPass();
            else
                renderPass = new RenderPass(chosenFormat.format);

            RenderPass::SetActive(renderPass);
        }

        inline void RefreshPipelineObjects() {
            vkDeviceWaitIdle(VirtualDevice::GetActive()->GetData());

            //DELETE PipelineObjects
			delete swapchain;
            delete renderPass;

            //DELETE PipelineAttachments
            delete depthImage;
            delete depthImageView;

            if (customRenderer != nullptr) {
                this->customRenderer->Refresh();
            }

            this->InitializePipelineObjects();
            this->InitializePipelineAttachments();
            this->swapchain->InitializeFramebuffers({depthImageView->GetData()}, renderPass);
        }

        inline void InitializePipelineAttachments()
        {
            //==============DEPTH=================//
            auto depthformat = PhysicalDevice::GetActive()->FindSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            depthImage = new Image(this->x, this->y, depthformat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            depthImageView = new ImageView(depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);

            depthImage->transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        inline ~Instance()
        {
            vkDeviceWaitIdle(VirtualDevice::GetActive()->GetData());

            if (enableValidationLayers) {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->data, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(this->data, this->debugMessenger, nullptr);
                }
            }

            delete surface;
            delete physicalDevice;
            delete virtualDevice;
            delete swapchain;
            delete renderPass;
            delete depthImage;
            delete depthImageView;
            delete commandPool;
            delete customRenderer;

            for (size_t i = 0; i < commandBuffers.size(); i++)
            {
                delete commandBuffers[i];
            }
            for (size_t i = 0; i < frameSynchronizationObjects.size(); i++)
            {
                delete frameSynchronizationObjects[i];
            }

            vkDestroyInstance(this->data, nullptr);
        }

        inline void PrepareFrame() {
            //==============SYNCING================
            vkWaitForFences(VirtualDevice::GetActive()->GetData(), 1, frameSynchronizationObjects[currentFrame]->Get_inFlightFence_ptr(), VK_TRUE, UINT64_MAX);

            VkResult aquireResult = vkAcquireNextImageKHR(VirtualDevice::GetActive()->GetData(), SwapChain::GetActive()->GetData(), UINT64_MAX, frameSynchronizationObjects[currentFrame]->Get_imageAvailableSemaphore(), VK_NULL_HANDLE, &currentSwapchainImage);

            if (aquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
                this->RefreshPipelineObjects();
                return;
            }
            else if (aquireResult != VK_SUCCESS && aquireResult != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            vkResetFences(VirtualDevice::GetActive()->GetData(), 1, frameSynchronizationObjects[currentFrame]->Get_inFlightFence_ptr());

            //==============COMMAND BUFFER START================
            vkResetCommandBuffer(commandBuffers[currentFrame]->GetData(), 0);
            commandBuffers[currentFrame]->Begin();

            //==============RENDER PASS START================
            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass->GetData();
            renderPassBeginInfo.framebuffer = swapchain->GetFramebuffer(currentSwapchainImage)->GetData();

            renderPassBeginInfo.renderArea.offset = { 0, 0 };
            renderPassBeginInfo.renderArea.extent = swapchain->GetExtent();

            std::array<VkClearValue, 2> clearValues{};
            float clear_brightness = 0.3f;
            clearValues[0].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[currentFrame]->GetData(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            //BOUNDS
            VkViewport viewport{};
            viewport.width = static_cast<float>(vulkan::SwapChain::GetActive()->GetExtent().width);
            viewport.height = static_cast<float>(vulkan::SwapChain::GetActive()->GetExtent().height);
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(this->GetCurrentCommandBuffer()->GetData(), 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = vulkan::SwapChain::GetActive()->GetExtent();
            vkCmdSetScissor(this->GetCurrentCommandBuffer()->GetData(), 0, 1, &scissor);
        }

        inline void PushFrame() {
            vkCmdEndRenderPass(commandBuffers[currentFrame]->GetData());
            commandBuffers[currentFrame]->End();

			//DEALLOCATE RESOURCES HERE IF NEEDED
            for (size_t i = 0; i < bufferDeletionQueue.size(); i++)
            {
				if (bufferDeletionQueue.front()->GetFrameIndexBelongsTo() == currentFrame) {
					bufferDeletionQueue[i]->UnbindFrame();
					delete bufferDeletionQueue[i];
					bufferDeletionQueue.erase(bufferDeletionQueue.begin() + i);
					i--;
				}
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { frameSynchronizationObjects[currentFrame]->Get_imageAvailableSemaphore()};
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = commandBuffers[currentFrame]->GetDataPtr();

            VkSemaphore signalSemaphores[] = { frameSynchronizationObjects[currentFrame]->Get_renderFinishedSemaphore(currentSwapchainImage)};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            CheckSuccess(vkQueueSubmit(virtualDevice->Get_graphicsQueue(), 1, &submitInfo, frameSynchronizationObjects[currentFrame]->Get_inFlightFence()));

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = { swapchain->GetData()};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &currentSwapchainImage;

            presentInfo.pResults = nullptr; // Optional

            VkResult presentResult = vkQueuePresentKHR(virtualDevice->Get_presentQueue(), &presentInfo);

            if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
                this->RefreshPipelineObjects();
            }
            else if (presentResult != VK_SUCCESS) {
                throw std::runtime_error("failed to present swap chain image!");
            }

            this->currentFrame = (this->currentFrame + 1) % this->MAX_FRAMES_IN_FLIGHT;
        }

    };
}