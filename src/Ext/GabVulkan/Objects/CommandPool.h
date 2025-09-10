#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

namespace gbe::vulkan {
	class CommandPool : public VulkanObject<VkCommandPool>, public VulkanObjectSingleton<CommandPool> {

	public:

		inline void RegisterDependencies() override {

		}

		inline CommandPool() {
			if (PhysicalDevice::GetActive() == nullptr)
				return;

			if (VirtualDevice::GetActive() == nullptr)
				return;

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = PhysicalDevice::GetActive()->Get_graphicsQueueIndex();

			CheckSuccess(vkCreateCommandPool(VirtualDevice::GetActive()->GetData(), &poolInfo, nullptr, &this->data));
			initialized = true;
		}
	};
}