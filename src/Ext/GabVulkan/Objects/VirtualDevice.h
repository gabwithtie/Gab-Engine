#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include "PhysicalDevice.h"

namespace gbe::vulkan {
    class VirtualDevice : public VulkanObject<VkDevice, VirtualDevice>, public VulkanObjectSingleton<VirtualDevice> {
        VkQueue graphicsQueue;
        VkQueue presentQueue;

    public:
        inline VkQueue Get_graphicsQueue() {
            return graphicsQueue;
        }

        inline VkQueue Get_presentQueue() {
            return presentQueue;
        }

        inline void RegisterDependencies() override {

        }

        inline ~VirtualDevice(){
            vkDestroyDevice(this->data, nullptr);
        }
        inline VirtualDevice(PhysicalDevice* from, const std::vector<const char*> deviceExtensionNames) {

            float queuePriority = 1.0f;
            VkDeviceQueueCreateInfo queueInfo = {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                nullptr,                                    // pNext
                0,                                          // flags
                from->Get_graphicsQueueIndex(),                         // graphicsQueueIndex
                1,                                          // queueCount
                &queuePriority,                             // pQueuePriorities
            };

            //FEATURES SETUP
            VkPhysicalDeviceFeatures deviceFeatures = {};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceFeatures.fillModeNonSolid = VK_TRUE;

            VkPhysicalDeviceVulkan11Features _VkPhysicalDeviceVulkan11Features = {};
            _VkPhysicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            _VkPhysicalDeviceVulkan11Features.shaderDrawParameters = VK_TRUE;
            _VkPhysicalDeviceVulkan11Features.pNext = nullptr;

            VkDeviceCreateInfo deviceInfo = {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,               // sType
                & _VkPhysicalDeviceVulkan11Features,                   // pNext
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
            vkCreateDevice(PhysicalDevice::GetActive()->GetData(), &deviceInfo, nullptr, &this->data);

            vkGetDeviceQueue(this->data, from->Get_graphicsQueueIndex(), 0, &graphicsQueue);
            vkGetDeviceQueue(this->data, from->Get_presentQueueIndex(), 0, &presentQueue);

        }

        inline void MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
        {
            vkMapMemory(this->data, memory, offset, size, flags, ppData);
        }

        inline void UnMapMemory(VkDeviceMemory memory)
        {
            vkUnmapMemory(this->data, memory);
        }

        inline void DeviceWaitIdle() {
			vkDeviceWaitIdle(this->data);
        }
    };
}