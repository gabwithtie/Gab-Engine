#pragma once

#include "../VulkanObject.h"

namespace gbe::vulkan {
    class Instance : public VulkanObject<VkInstance> {
    public:
        //INSTANCE INFO
        VkSurfaceKHR vksurface;
        VkDebugUtilsMessengerEXT debugMessenger;
        uint32_t graphicsQueueIndex = UINT32_MAX;
        uint32_t presentQueueIndex = UINT32_MAX;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        //SWAPCHAINS
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkSurfaceFormatKHR chosenSurfaceFormat;
        VkExtent2D swapchainExtent;
        std::vector<VkImageView> swapChainImageViews;
        //SWAPCHAIN FRAMEBUFFERS
        std::vector<VkFramebuffer> swapChainFramebuffers;

        //Renderpass
        VkRenderPass renderPass;

        //DEPTH PASS
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

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

        inline void Init() {
            //===================DEVICE SET UP===================//
    //EXTENSIONS
            const std::vector<const char*> deviceExtensionNames = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_MAINTENANCE1_EXTENSION_NAME
            };

            bool founddevice = false;
            uint32_t physicalDeviceCount;
            vkEnumeratePhysicalDevices(vkInst, &physicalDeviceCount, nullptr);
            if (physicalDeviceCount == 0) {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(vkInst, &physicalDeviceCount, physicalDevices.data());
            //TEST DEVICE SUITABLE
            const auto isDeviceSuitable = [=](VkPhysicalDevice vkpdevice) {
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(vkpdevice, nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(vkpdevice, nullptr, &extensionCount, availableExtensions.data());

                std::set<std::string> requiredExtensions(deviceExtensionNames.begin(), deviceExtensionNames.end());

                for (const auto& extension : availableExtensions) {
                    requiredExtensions.erase(extension.extensionName);
                }
                bool extensionsSupported = requiredExtensions.empty();

                bool swapChainAdequate = false;
                if (extensionsSupported) {
                    VkSurfaceCapabilitiesKHR capabilities = {};
                    std::vector<VkSurfaceFormatKHR> formats;
                    std::vector<VkPresentModeKHR> presentModes;
                    querySwapChainSupport(vkpdevice, this->vksurface, capabilities, formats, presentModes);
                    swapChainAdequate = !formats.empty() && !presentModes.empty();
                }

                VkPhysicalDeviceFeatures supportedFeatures;
                vkGetPhysicalDeviceFeatures(vkpdevice, &supportedFeatures);

                return extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
                };
            for (const auto& vkpdevice : physicalDevices) {
                if (isDeviceSuitable(vkpdevice)) {
                    this->vkphysicalDevice = vkpdevice;
                    founddevice = true;
                    break;
                }
            }

            if (founddevice == false) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }

            uint32_t queueFamilyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(this->vkphysicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(this->vkphysicalDevice, &queueFamilyCount, queueFamilies.data());

            VkBool32 support;
            uint32_t i = 0;
            for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
                if (graphicsQueueIndex == UINT32_MAX && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    graphicsQueueIndex = i;
                if (presentQueueIndex == UINT32_MAX) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(this->vkphysicalDevice, i, vksurface, &support);
                    if (support)
                        presentQueueIndex = i;
                }
                ++i;
            }

            float queuePriority = 1.0f;
            VkDeviceQueueCreateInfo queueInfo = {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                nullptr,                                    // pNext
                0,                                          // flags
                graphicsQueueIndex,                         // graphicsQueueIndex
                1,                                          // queueCount
                &queuePriority,                             // pQueuePriorities
            };

            //FEATURES SETUP
            VkPhysicalDeviceFeatures deviceFeatures = {};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceFeatures.fillModeNonSolid = VK_TRUE;

            VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
            shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
            shader_draw_parameters_features.pNext = nullptr;
            shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;

            VkDeviceCreateInfo deviceInfo = {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,               // sType
                &shader_draw_parameters_features,                   // pNext
                0,                                                  // flags
                1,                                                  // queueCreateInfoCount
                &queueInfo,                                         // pQueueCreateInfos
                0,                                                  // enabledLayerCount
                nullptr,                                            // ppEnabledLayerNames
                static_cast<uint32_t>(deviceExtensionNames.size()), // enabledExtensionCount
                deviceExtensionNames.data(),                        // ppEnabledExtensionNames
                &deviceFeatures,                                    // pEnabledFeatures
            };
            //DEVICE
            vkCreateDevice(this->vkphysicalDevice, &deviceInfo, nullptr, &vkdevice);

            vkGetDeviceQueue(vkdevice, graphicsQueueIndex, 0, &graphicsQueue);
            vkGetDeviceQueue(vkdevice, presentQueueIndex, 0, &presentQueue);

            if (graphicsQueueIndex == UINT32_MAX || presentQueueIndex == UINT32_MAX) {
                throw std::runtime_error("failed to find a suitable queue family!");
            }
#pragma endregion

            this->InitializePipelineObjects();

#pragma region command pool
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = graphicsQueueIndex;

            if (vkCreateCommandPool(this->vkdevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create command pool!");
            }

            commandBuffers.resize(this->MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

            if (vkAllocateCommandBuffers(this->vkdevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
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
                if (vkCreateSemaphore(this->vkdevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(this->vkdevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(this->vkdevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                    throw std::runtime_error("failed to create synchronization objects for a frame!");
                }
            }
#pragma endregion
        }

        inline void InitializePipelineObjects() {

#pragma region swapchain init
            VkSurfaceCapabilitiesKHR capabilities = {};
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;

            querySwapChainSupport(this->vkphysicalDevice, vksurface, capabilities, formats, presentModes);

            //format selection
            for (const auto& availableFormat : formats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    this->chosenSurfaceFormat = availableFormat;
                    break;
                }
            }

            this->chosenSurfaceFormat = formats[0];

            //presentation mode selection
            VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

            for (const auto& availablePresentMode : presentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    chosenPresentMode = availablePresentMode;
                }
            }

            //swapchain extent selection
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                this->swapchainExtent = capabilities.currentExtent;
            }
            else {
                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(this->resolution.x),
                    static_cast<uint32_t>(this->resolution.y)
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                this->swapchainExtent = actualExtent;
            }

            uint32_t imageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
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

            uint32_t queueFamilyIndices[] = { graphicsQueueIndex, presentQueueIndex };

            if (graphicsQueueIndex != presentQueueIndex) {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainInfo.queueFamilyIndexCount = 2;
                swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapchainInfo.queueFamilyIndexCount = 0; // Optional
                swapchainInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            swapchainInfo.preTransform = capabilities.currentTransform;
            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainInfo.presentMode = chosenPresentMode;
            swapchainInfo.clipped = VK_TRUE;
            swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(vkdevice, &swapchainInfo, nullptr, &swapChain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }

            //Retrieving the swap chain images
            vkGetSwapchainImagesKHR(vkdevice, this->swapChain, &imageCount, nullptr);
            this->swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(vkdevice, this->swapChain, &imageCount, swapChainImages.data());

            //Image views
            swapChainImageViews.resize(swapChainImages.size());

            for (uint32_t i = 0; i < swapChainImages.size(); i++) {
                createImageView(swapChainImageViews[i], swapChainImages[i], chosenSurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
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
            depthAttachment.format = findSupportedFormat(
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

            if (vkCreateRenderPass(this->vkdevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
#pragma endregion
        }

        inline void RefreshPipelineObjects() {
            if (this->window->isMinimized())
                return;

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
                    swapChainImageViews[i],
                    depthImageView
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = swapchainExtent.width;
                framebufferInfo.height = swapchainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(this->vkdevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }

        inline void CleanPipelineObjects() {
            vkDeviceWaitIdle(this->vkdevice);

            vkDestroyImageView(this->vkdevice, depthImageView, nullptr);
            vkDestroyImage(this->vkdevice, depthImage, nullptr);
            vkFreeMemory(this->vkdevice, depthImageMemory, nullptr);

            for (auto framebuffer : swapChainFramebuffers) {
                vkDestroyFramebuffer(this->vkdevice, framebuffer, nullptr);
            }

            for (auto imageView : swapChainImageViews) {
                vkDestroyImageView(vkdevice, imageView, nullptr);
            }

            vkDestroySwapchainKHR(vkdevice, swapChain, nullptr);
        }

        inline void CreateDepthResources()
        {
            auto depthformat = findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            createImage(this->resolution.x, this->resolution.y, depthformat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
            createImageView(depthImageView, depthImage, depthformat, VK_IMAGE_ASPECT_DEPTH_BIT);

            transitionImageLayout(depthImage, depthformat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        }

        inline void CleanUp()
        {
            if (enableValidationLayers) {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->vkInst, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(this->vkInst, this->debugMessenger, nullptr);
                }
            }

            vkDeviceWaitIdle(this->vkdevice);

            this->CleanPipelineObjects();

            //Insert Shader Program disposal here

            vkDestroyRenderPass(vkdevice, renderPass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(this->vkdevice, renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(this->vkdevice, imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(this->vkdevice, inFlightFences[i], nullptr);
            }
            vkDestroyCommandPool(this->vkdevice, commandPool, nullptr);

            for (auto it = drawcalls.begin(); it != drawcalls.end(); it++) {
                const auto& drawcallbatch = it->second;

                for (const auto& drawcall : drawcallbatch)
                {
                    delete drawcall;
                }
            }

            vkDestroySurfaceKHR(vkInst, vksurface, nullptr);
            vkDestroyDevice(this->vkdevice, nullptr);
            vkDestroyInstance(vkInst, nullptr);
        }

    };
}