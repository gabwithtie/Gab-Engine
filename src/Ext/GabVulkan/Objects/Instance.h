#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include "../Utility/DebugCallback.h"

#include <algorithm>
#include <array>
#include <vector>

#include "Image.h"
#include "ImageView.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "PhysicalDevice.h"
#include "VirtualDevice.h"

namespace gbe::vulkan {
    class Instance : public VulkanObject<VkInstance>, public VulkanObjectSingleton<Instance> {
    
    public:
        //INSTANCE INFO
        VkSurfaceKHR vksurface = nullptr;
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        bool enableValidationLayers = false;

        //DEVICES
        PhysicalDevice physicalDevice;
        VirtualDevice virtualDevice;

        //SWAPCHAINS
        unsigned int x;
        unsigned int y;
        vulkan::SwapChain swapchain;

        //Renderpass
        RenderPass renderPass;

        //DEPTH PASS
        Image depthImage;
        ImageView depthImageView;

        //COMMANDPOOL
        CommandPool commandPool;
        const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t currentFrame = 0;
        std::vector<VkCommandBuffer> commandBuffers;

        //Synchronization
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        inline void RegisterDependencies() override {

        }

        inline Instance() {

        }

        inline Instance(unsigned int _x, unsigned int _y, std::vector<const char*>& allextensions, bool _enableValidationLayers, const std::vector<const char*>& validationLayers) {
            this->x = _x;
            this->y = _y;
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
                instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                instInfo.ppEnabledLayerNames = validationLayers.data();

                debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debug_messenger_create_info.pfnUserCallback = UserDebugCallback;
                debug_messenger_create_info.pUserData = nullptr; // Optional

                instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;
            }
            else {
                instInfo.enabledLayerCount = 0;

                instInfo.pNext = nullptr;
            }
            if (enableValidationLayers) {
                auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->data, "vkCreateDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(this->data, &debug_messenger_create_info, nullptr, &debugMessenger);
                }
                else {
                    throw std::runtime_error("failed to set up debug messenger!");
                }
            }

            //INSTANCE
            CheckSuccess(vkCreateInstance(&instInfo, nullptr, &this->data));
            initialized = true;
        }

        inline void Init(){
            //===================DEVICE SET UP===================//
            const std::vector<const char*> deviceExtensionNames = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_MAINTENANCE1_EXTENSION_NAME
            };

            bool founddevice = false;
            uint32_t physicalDeviceCount;
            vkEnumeratePhysicalDevices(this->data, &physicalDeviceCount, nullptr);
            if (physicalDeviceCount == 0) {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(this->data, &physicalDeviceCount, physicalDevices.data());
            std::vector<PhysicalDevice> gbephysicalDevices;
            for (const auto pd : physicalDevices)
            {
                gbephysicalDevices.push_back(PhysicalDevice(pd));
            }

            //TEST DEVICE SUITABLE
            std::set<std::string> requiredExtensions(deviceExtensionNames.begin(), deviceExtensionNames.end());

            for (auto& vkpdevice : gbephysicalDevices) {
                if (vkpdevice.IsCompatible(requiredExtensions) && vkpdevice.Get_Features().samplerAnisotropy) {
                    physicalDevice = vkpdevice;
                    PhysicalDevice::SetActive(&physicalDevice);
                    founddevice = true;
                    break;
                }
            }

            if (founddevice == false) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }

            virtualDevice = VirtualDevice(PhysicalDevice::GetActive(), deviceExtensionNames);
            VirtualDevice::SetActive(&virtualDevice);

            //======================OBJECT SETUP========================
            this->InitializePipelineObjects();

            //======================CommandPool SETUP========================
            commandPool = CommandPool();
            CommandPool::SetActive(&commandPool);

            commandBuffers.resize(this->MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool.GetData();
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

            if (vkAllocateCommandBuffers(VirtualDevice::GetActive()->GetData(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffers!");
            }

            //======================DISPLAY SETUP========================
            this->CreateDepthResources();
            this->swapchain.InitializeFramebuffers(depthImageView, renderPass);

            //======================SYNCHRONIZATION SETUP========================
            this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (vkCreateSemaphore(VirtualDevice::GetActive()->GetData(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(VirtualDevice::GetActive()->GetData(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(VirtualDevice::GetActive()->GetData(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                    throw std::runtime_error("failed to create synchronization objects for a frame!");
                }
            }
        }

        inline void InitializePipelineObjects() {
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
            VkExtent2D swapchainExtent;

            if (PhysicalDevice::GetActive()->Get_capabilities().currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                swapchainExtent = PhysicalDevice::GetActive()->Get_capabilities().currentExtent;
            }
            else {
                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(this->x),
                    static_cast<uint32_t>(this->y)
                };

                actualExtent.width = std::clamp(actualExtent.width, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.width, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.height, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.height);

                swapchainExtent = actualExtent;
            }

            //Image count determination
            uint32_t imageCount = PhysicalDevice::GetActive()->Get_capabilities().minImageCount + 1;
            if (PhysicalDevice::GetActive()->Get_capabilities().maxImageCount > 0 && imageCount > PhysicalDevice::GetActive()->Get_capabilities().maxImageCount) {
                imageCount = PhysicalDevice::GetActive()->Get_capabilities().maxImageCount;
            }

            swapchain = SwapChain(chosenFormat, chosenPresentMode, swapchainExtent, imageCount);
            SwapChain::SetActive(&swapchain);

            //==============RENDERPASS================
            renderPass = RenderPass(chosenFormat.format);
            RenderPass::SetActive(&renderPass);
        }

        inline void RefreshPipelineObjects() {
            this->CleanPipelineObjects();
            this->InitializePipelineObjects();
            this->CreateDepthResources();
            this->swapchain.InitializeFramebuffers(depthImageView, renderPass);
        }

        inline void CleanPipelineObjects() {
            vkDeviceWaitIdle(VirtualDevice::GetActive()->GetData());


        }

        inline void CreateDepthResources()
        {
            auto depthformat = PhysicalDevice::GetActive()->FindSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            depthImage = Image(this->x, this->y, depthformat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            depthImageView = ImageView(depthImage, VK_IMAGE_ASPECT_DEPTH_BIT);

            depthImage.transitionImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        inline void CleanUp()
        {
            if (enableValidationLayers) {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->data, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(this->data, this->debugMessenger, nullptr);
                }
            }

            vkDeviceWaitIdle(VirtualDevice::GetActive()->GetData());

            this->CleanPipelineObjects();

            //Insert Shader Program disposal here
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(VirtualDevice::GetActive()->GetData(), renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(VirtualDevice::GetActive()->GetData(), imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(VirtualDevice::GetActive()->GetData(), inFlightFences[i], nullptr);
            }
            vkDestroyCommandPool(VirtualDevice::GetActive()->GetData(), CommandPool::GetActive()->GetData(), nullptr);

            vkDestroySurfaceKHR(this->data, vksurface, nullptr);
            vkDestroyDevice(VirtualDevice::GetActive()->GetData(), nullptr);
            vkDestroyInstance(this->data, nullptr);
        }

    };
}