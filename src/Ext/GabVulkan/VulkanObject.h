#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

namespace gbe::vulkan {
	class VulkanObject_base {

	};

	template<typename T>
	class VulkanObject : VulkanObject_base {
	private:
		std::vector<VulkanObject_base*> dependents;
		std::vector<VulkanObject_base*> dependencies;

	protected:
		T data;
		inline void CheckSuccess(VkResult result) {
			if (result != VK_SUCCESS) {
				throw std::runtime_error("Vulkan call failed with error code: " + result);
			}
		}
		bool initialized = false;
	public:
		inline virtual T GetData() const {
			return data;
		};
		inline virtual void RegisterDependencies() = 0;
	};
}