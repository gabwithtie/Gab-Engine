#pragma once

#include "../VulkanObject.h"

namespace gbe::vulkan {
	class PhysicalDevice : public VulkanObject<VkPhysicalDevice> {
		static PhysicalDevice _Active;

		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	
		inline void QuerySwapChainSupport(VkSurfaceKHR surface)
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->data, surface, &capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(this->data, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(this->data, surface, &formatCount, formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(this->data, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(this->data, surface, &presentModeCount, presentModes.data());
			}
		}

	public:
		inline VkSurfaceCapabilitiesKHR Get_capabilities() {
			return capabilities;
		}
		inline std::vector<VkSurfaceFormatKHR> Get_formats() {
			return formats;
		}
		inline std::vector<VkPresentModeKHR> Get_presentModes() {
			return presentModes;
		}

		inline static PhysicalDevice Active() {
			return _Active;
		}
		inline static PhysicalDevice SetActiveDevice(PhysicalDevice newactive) {
			_Active = newactive;
		}
		inline void RegisterDependencies() override {

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

		inline VkFormat gbe::RenderPipeline::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : candidates) {
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(this->vkphysicalDevice, format, &props);

				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					return format;
				}
			}

			throw std::runtime_error("failed to find supported format!");
		}
	};
}