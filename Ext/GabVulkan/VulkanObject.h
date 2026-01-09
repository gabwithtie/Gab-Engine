#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

namespace gbe::vulkan {
	class VulkanObject_base {

	};

	template<typename Vk, class Derived>
	class VulkanObject : VulkanObject_base {
	protected:
		Vk data;
		inline void CheckSuccess(VkResult result) {
			if (result != VK_SUCCESS) {
				throw std::runtime_error("Vulkan call failed with error code: " + result);
			}
		}

	public:
		inline Vk GetData() const {
			return data;
		};
		inline Vk* GetDataPtr() {
			return &data;
		}

		inline virtual void RegisterDependencies() = 0;
	};
}