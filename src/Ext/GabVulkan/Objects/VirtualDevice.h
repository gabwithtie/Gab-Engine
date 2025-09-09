#pragma once

#include "../VulkanObject.h"

namespace gbe::vulkan {
	class VirtualDevice : public VulkanObject<VkDevice> {
		static VirtualDevice _Active;

	public:
		inline static VirtualDevice Active() {
			return _Active;
		}
		inline static VirtualDevice SetActiveDevice(VirtualDevice newactive) {
			_Active = newactive;
		}
		inline void RegisterDependencies() override {

		}
	};
}