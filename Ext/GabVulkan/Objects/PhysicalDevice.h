#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

#include <set>

#include "Surface.h"

namespace gbe::vulkan {
	class PhysicalDevice : public VulkanObject<VkPhysicalDevice, PhysicalDevice>, public VulkanObjectSingleton<PhysicalDevice> {
		uint32_t graphicsQueueIndex = UINT32_MAX;
		uint32_t presentQueueIndex = UINT32_MAX;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures supportedFeatures;
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		VkSurfaceFormatKHR swapchainFormat;
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	public:
		inline VkSurfaceFormatKHR Get_swapchainFormat() {
			return swapchainFormat;
		}

		inline VkPresentModeKHR Get_swapchainPresentMode() {
			return swapchainPresentMode;
		}

		inline VkSurfaceCapabilitiesKHR Get_capabilities() const {
			return capabilities;
		}
		inline std::vector<VkSurfaceFormatKHR> Get_formats() const {
			return formats;
		}
		inline std::vector<VkPresentModeKHR> Get_presentModes() const {
			return presentModes;
		}
		inline const VkPhysicalDeviceFeatures Get_Features() const {
			return supportedFeatures;
		}
		inline const VkPhysicalDeviceProperties Get_properties() const {
			return properties;
		}
		inline uint32_t Get_graphicsQueueIndex() const {
			return graphicsQueueIndex;
		}
		inline uint32_t Get_presentQueueIndex() const {
			return presentQueueIndex;
		}
		inline void Refresh() {
			vkGetPhysicalDeviceProperties(this->data, &properties);
			vkGetPhysicalDeviceFeatures(this->data, &supportedFeatures);
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->data, Surface::GetActive()->GetData(), &capabilities);
		}

		inline void RegisterDependencies() override {

		}

		inline PhysicalDevice() {

		}

		inline PhysicalDevice(VkPhysicalDevice vkphysicaldevice) {
			this->data = vkphysicaldevice;

			Refresh();

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(this->data, Surface::GetActive()->GetData(), &formatCount, nullptr);
			if (formatCount != 0) {
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(this->data, Surface::GetActive()->GetData(), &formatCount, formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(this->data, Surface::GetActive()->GetData(), &presentModeCount, nullptr);
			if (presentModeCount != 0) {
				presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(this->data, Surface::GetActive()->GetData(), &presentModeCount, presentModes.data());
			}

			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(this->data, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(this->data, &queueFamilyCount, queueFamilies.data());

			VkBool32 support;
			uint32_t i = 0;
			for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
				if (graphicsQueueIndex == UINT32_MAX && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					graphicsQueueIndex = i;
				if (presentQueueIndex == UINT32_MAX) {
					vkGetPhysicalDeviceSurfaceSupportKHR(this->data, i, Surface::GetActive()->GetData(), &support);
					if (support)
						presentQueueIndex = i;
				}
				++i;
			}
			if (graphicsQueueIndex == UINT32_MAX || presentQueueIndex == UINT32_MAX) {
				throw std::runtime_error("failed to find a suitable queue family!");
			}

			//format selection
			for (const auto& availableFormat : formats) {
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					swapchainFormat = availableFormat;
					break;
				}
			}
			//presentation mode selection
			for (const auto& availablePresentMode : presentModes) {
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
					swapchainPresentMode = availablePresentMode;
				}
			}
		}

		inline bool IsCompatible(std::set<std::string> requiredExtensions) const {
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(this->data, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(this->data, nullptr, &extensionCount, availableExtensions.data());

			for (const auto& extension : availableExtensions) {
				requiredExtensions.erase(extension.extensionName);
			}
			bool extensionsSupported = requiredExtensions.empty();

			if (extensionsSupported) {
				return !formats.empty() && !presentModes.empty();
			}

			return false;
		}

		inline uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(data, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			throw std::runtime_error("failed to find suitable memory type!");
		}

		inline VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : candidates) {
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(this->data, format, &props);

				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					return format;
				}
			}

			throw std::runtime_error("failed to find supported format!");
		}

		inline VkFormat GetDepthFormat() {
			return FindSupportedFormat(
				{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
		}
	};
}