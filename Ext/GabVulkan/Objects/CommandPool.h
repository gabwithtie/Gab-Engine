#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"
#include "VirtualDevice.h"

namespace gbe::vulkan {
	class CommandPool : public VulkanObject<VkCommandPool, CommandPool>, public VulkanObjectSingleton<CommandPool> {
	protected:
	public:
		inline ~CommandPool(){
			vkDestroyCommandPool(VirtualDevice::GetActive()->GetData(), CommandPool::GetActive()->GetData(), nullptr);
		}

		inline void RegisterDependencies() override {

		}


		inline CommandPool() {
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = PhysicalDevice::GetActive()->Get_graphicsQueueIndex();

			CheckSuccess(vkCreateCommandPool(VirtualDevice::GetActive()->GetData(), &poolInfo, nullptr, &this->data));
		}
	};
}