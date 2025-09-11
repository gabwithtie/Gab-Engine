#pragma once

#include <vulkan/vulkan.h>

#include "../VirtualDevice.h"

namespace gbe::vulkan {
	class FrameSyncronizationObject {
	private:
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;

	public:
		VkSemaphore* Get_imageAvailableSemaphore_ptr() {return &imageAvailableSemaphore;}
		VkSemaphore* Get_renderFinishedSemaphore_ptr() {return &renderFinishedSemaphore;}
		VkFence* Get_inFlightFence_ptr() {return &inFlightFence;}

		VkSemaphore Get_imageAvailableSemaphore() const { return imageAvailableSemaphore; }
		VkSemaphore Get_renderFinishedSemaphore() const { return renderFinishedSemaphore; }
		VkFence Get_inFlightFence() const { return inFlightFence; }

		inline ~FrameSyncronizationObject() {
			vkDestroySemaphore(VirtualDevice::GetActive()->GetData(), renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(VirtualDevice::GetActive()->GetData(), imageAvailableSemaphore, nullptr);
			vkDestroyFence(VirtualDevice::GetActive()->GetData(), inFlightFence, nullptr);
		}

		inline FrameSyncronizationObject() {
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateSemaphore(VirtualDevice::GetActive()->GetData(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
				vkCreateSemaphore(VirtualDevice::GetActive()->GetData(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
				vkCreateFence(VirtualDevice::GetActive()->GetData(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	};
}