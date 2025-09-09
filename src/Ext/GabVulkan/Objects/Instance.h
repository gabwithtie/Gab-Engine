#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include "../Utility/DebugCallback.h"

#include <algorithm>
#include <array>

#include "Image.h"
#include "ImageView.h"

namespace gbe::vulkan {
    class Instance : public VulkanObject<VkInstance>, public VulkanObjectSingleton<Instance> {
    
    public:
        //INSTANCE INFO
        VkSurfaceKHR vksurface;
        VkDebugUtilsMessengerEXT debugMessenger;
        bool enableValidationLayers = false;

        //SWAPCHAINS
        unsigned int x;
        unsigned int y;

        VkSwapchainKHR swapChain;
        std::vector<Image> swapChainImages;
        VkSurfaceFormatKHR chosenSurfaceFormat;
        VkExtent2D swapchainExtent;
        std::vector<ImageView> swapChainImageViews;
        //SWAPCHAIN FRAMEBUFFERS
        std::vector<VkFramebuffer> swapChainFramebuffers;

        //Renderpass
        VkRenderPass renderPass;

        //DEPTH PASS
        Image depthImage;
        ImageView depthImageView;

        //COMMANDPOOL
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

        inline Instance(unsigned int _x, unsigned int _y, std::vector<const char*> allextensions, bool _enableValidationLayers = false, std::vector<const char*>* validationLayers = nullptr) {
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
                instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers->size());
                instInfo.ppEnabledLayerNames = validationLayers->data();

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
    //EXTENSIONS
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
                    PhysicalDevice::SetActive(&vkpdevice);
                    founddevice = true;
                    break;
                }
            }

            if (founddevice == false) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }

            VirtualDevice virtualdevice(PhysicalDevice::GetActive(), deviceExtensionNames);
            VirtualDevice::SetActive(&virtualdevice);

#pragma endregion

            this->InitializePipelineObjects();

#pragma region command pool
            CommandPool commandPool;
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
#pragma endregion

            this->CreateDepthResources();
            this->InitializeFramebuffers();

#pragma region synchronization
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
#pragma endregion
        }

        inline void InitializePipelineObjects() {

#pragma region swapchain init
            //format selection
            for (const auto& availableFormat : PhysicalDevice::GetActive()->Get_formats()) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    this->chosenSurfaceFormat = availableFormat;
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
            if (PhysicalDevice::GetActive()->Get_capabilities().currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                this->swapchainExtent = PhysicalDevice::GetActive()->Get_capabilities().currentExtent;
            }
            else {
                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(this->x),
                    static_cast<uint32_t>(this->y)
                };

                actualExtent.width = std::clamp(actualExtent.width, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.width, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, PhysicalDevice::GetActive()->Get_capabilities().minImageExtent.height, PhysicalDevice::GetActive()->Get_capabilities().maxImageExtent.height);

                this->swapchainExtent = actualExtent;
            }

            uint32_t imageCount = PhysicalDevice::GetActive()->Get_capabilities().minImageCount + 1;
            if (PhysicalDevice::GetActive()->Get_capabilities().maxImageCount > 0 && imageCount > PhysicalDevice::GetActive()->Get_capabilities().maxImageCount) {
                imageCount = PhysicalDevice::GetActive()->Get_capabilities().maxImageCount;
            }

            VkSwapchainCreateInfoKHR swapchainInfo{};
            swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainInfo.surface = vksurface;

            swapchainInfo.minImageCount = imageCount;
            swapchainInfo.imageFormat = this->chosenSurfaceFormat.format;
            swapchainInfo.imageColorSpace = this->chosenSurfaceFormat.colorSpace;
            swapchainInfo.imageExtent = this->swapchainExtent;
            swapchainInfo.imageArrayLayers = 1;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            uint32_t queueFamilyIndices[] = { PhysicalDevice::GetActive()->Get_graphicsQueueIndex(), PhysicalDevice::GetActive()->Get_presentQueueIndex()};

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

            if (vkCreateSwapchainKHR(VirtualDevice::GetActive()->GetData(), &swapchainInfo, nullptr, &swapChain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }

            //Retrieving the swap chain images
            vkGetSwapchainImagesKHR(VirtualDevice::GetActive()->GetData(), this->swapChain, &imageCount, nullptr);
            std::vector<VkImage> swapchain_vkimages(imageCount);
            vkGetSwapchainImagesKHR(VirtualDevice::GetActive()->GetData(), this->swapChain, &imageCount, swapchain_vkimages.data());

            for (auto& vkimg : swapchain_vkimages)
            {
                swapChainImages.push_back(Image(vkimg));
            }

            //Image views
            swapChainImageViews.resize(swapChainImages.size());

            for (uint32_t i = 0; i < swapChainImages.size(); i++) {
                swapChainImageViews[i] = ImageView(swapChainImages[i], VK_IMAGE_ASPECT_COLOR_BIT);
            }
#pragma endregion

#pragma region Render pass
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = this->chosenSurfaceFormat.format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = PhysicalDevice::GetActive()->FindSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;

            //Subpass dependencies
            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;

            dependency.srcAccessMask = 0;

            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            //Renderpass
            std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
#pragma endregion
        }

        inline void RefreshPipelineObjects() {
            this->CleanPipelineObjects();
            this->InitializePipelineObjects();
            this->CreateDepthResources();
            this->InitializeFramebuffers();
        }

        inline void InitializeFramebuffers()
        {
            this->swapChainFramebuffers.resize(this->swapChainImageViews.size());
            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                std::array<VkImageView, 2> attachments = {
                    swapChainImageViews[i].GetData(),
                    depthImageView.GetData()
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = swapchainExtent.width;
                framebufferInfo.height = swapchainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(VirtualDevice::GetActive()->GetData(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }

        inline void CleanPipelineObjects() {
            vkDeviceWaitIdle(VirtualDevice::GetActive()->GetData());

            for (auto framebuffer : swapChainFramebuffers) {
                vkDestroyFramebuffer(VirtualDevice::GetActive()->GetData(), framebuffer, nullptr);
            }

            for (auto imageView : swapChainImageViews) {
                vkDestroyImageView(VirtualDevice::GetActive()->GetData(), imageView.GetData(), nullptr);
            }

            vkDestroySwapchainKHR(VirtualDevice::GetActive()->GetData(), swapChain, nullptr);
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

            vkDestroyRenderPass(VirtualDevice::GetActive()->GetData(), renderPass, nullptr);

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