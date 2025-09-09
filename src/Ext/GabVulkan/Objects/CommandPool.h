#pragma once

#include "../VulkanObject.h"

namespace gbe::vulkan {
	class CommandPool : public VulkanObject<VkCommandPool> {
		static CommandPool _Active;

	public:
		inline static CommandPool Active() {
			return _Active;
		}
		inline static CommandPool SetActiveDevice(CommandPool newactive) {
			_Active = newactive;
		}

		inline void RegisterDependencies() override {

		}
	}
};