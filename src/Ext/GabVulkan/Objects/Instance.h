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
#include "Structures/AttachmentDictionary.h"


namespace gbe::vulkan {
    class Instance : public VulkanObject<VkInstance, Instance>, public VulkanObjectSingleton<Instance> {
    
        //INFOs
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        bool enableValidationLayers = false;

        int &x;
        int &y;
        int MAX_FRAMES_IN_FLIGHT = 1;
        
        //STATES
        uint32_t currentFrame = 0;
        uint32_t currentSwapchainImage = 0;
        
        //==============DYNAMICALLY ALLOCATED============
        Surface* surface = nullptr;
        PhysicalDevice* physicalDevice = nullptr;
        VirtualDevice* virtualDevice = nullptr;
        vulkan::SwapChain* swapchain = nullptr; //remember that images are arbitrary
        CommandPool* commandPool = nullptr;
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

        inline Framebuffer* GetCurrentSwapchainBuffer() {
            return swapchain->GetFramebuffer(currentSwapchainImage);
        }

        inline Surface* GetSurface() {
            return surface;
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

        inline void Init_Surface(Surface* _surface) {
            //===================SURFACE SET UP===================//

            surface = _surface;
            Surface::SetActive(surface);
            //reconstruct because surface is expected to be constructed using dereferencing.

            //===================DEVICE SET UP===================//
            const std::vector<const char*> deviceExtensionNames = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME
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

            //======================CommandPool SETUP========================
            commandPool = new CommandPool();
            CommandPool::SetActive(commandPool);

            commandBuffers.resize(this->MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < this->MAX_FRAMES_IN_FLIGHT; i++)
            {
                commandBuffers[i] = new CommandBuffer(commandPool->GetData());
            }
        }

        inline void Init_Renderer(Renderer* _customrenderer) {
            this->customRenderer = _customrenderer;
            
            //======================DISPLAY SETUP========================
            this->RefreshPipelineObjects();

            //======================SYNCHRONIZATION SETUP========================
            this->frameSynchronizationObjects.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                frameSynchronizationObjects[i] = new FrameSyncronizationObject(this->swapchain->GetImageCount());
            }
        }

        inline void RefreshPipelineObjects() {
            vkDeviceWaitIdle(VirtualDevice::GetActive()->GetData());

            //DELETE PipelineObjects
            if (swapchain != nullptr)
                delete swapchain;

            PhysicalDevice::GetActive()->Refresh();

            //==============SWAPCHAIN================
            //swapchain extent selection
            VkExtent2D swapchainExtent = {
                static_cast<uint32_t>(this->x),
                static_cast<uint32_t>(this->y)
            };

            swapchainExtent.width = std::clamp(swapchainExtent.width, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.width, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.width);
            swapchainExtent.height = std::clamp(swapchainExtent.height, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.height, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.height);

            if (customRenderer != nullptr) {
                this->customRenderer->Refresh(swapchainExtent.width, swapchainExtent.height);
            }

            //Image count determination
            uint32_t imageCount = PhysicalDevice::GetActive()->Get_capabilities().minImageCount + 1;
            if (PhysicalDevice::GetActive()->Get_capabilities().maxImageCount > 0 && imageCount > PhysicalDevice::GetActive()->Get_capabilities().maxImageCount) {
                imageCount = PhysicalDevice::GetActive()->Get_capabilities().maxImageCount;
            }

            swapchain = new SwapChain(swapchainExtent, imageCount);
            SwapChain::SetActive(swapchain);

            auto& attachmentdict = this->customRenderer->GetAttachmentDictionary();
            AttachmentReferencePasser newpasser(attachmentdict);

            this->customRenderer->PassAttachments(newpasser);

            this->swapchain->InitializeFramebuffers(newpasser, this->customRenderer->GetScreenPass());
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
        }

        inline void PushFrame() {
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

            //Image::copyImageToImage(depthImage, depthImage_reflect, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            this->currentFrame = (this->currentFrame + 1) % this->MAX_FRAMES_IN_FLIGHT;
        }

    };
}