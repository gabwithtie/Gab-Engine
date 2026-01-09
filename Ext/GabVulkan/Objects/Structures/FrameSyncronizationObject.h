#pragma once

#include <vulkan/vulkan.h>

#include "../VirtualDevice.h"

namespace gbe::vulkan {
	class FrameSyncronizationObject {
	private:
		VkSemaphore imageAvailableSemaphore;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		VkFence inFlightFence;

	public:
		VkSemaphore* Get_imageAvailableSemaphore_ptr() {return &imageAvailableSemaphore;}
		VkSemaphore* Get_renderFinishedSemaphore_ptr(int swapchainimageindex) {return &renderFinishedSemaphores[swapchainimageindex];}
		VkFence* Get_inFlightFence_ptr() {return &inFlightFence;}

		VkSemaphore Get_imageAvailableSemaphore() const { return imageAvailableSemaphore; }
		VkSemaphore Get_renderFinishedSemaphore(int swapchainimageindex) const { return renderFinishedSemaphores[swapchainimageindex]; }
		VkFence Get_inFlightFence() const { return inFlightFence; }

		inline ~FrameSyncronizationObject() {
			
			for (size_t i = 0; i < renderFinishedSemaphores.size(); i++)
			{
				vkDestroySemaphore(VirtualDevice::GetActive()->GetData(), renderFinishedSemaphores[i], nullptr);
			}

			vkDestroySemaphore(VirtualDevice::GetActive()->GetData(), imageAvailableSemaphore, nullptr);
			vkDestroyFence(VirtualDevice::GetActive()->GetData(), inFlightFence, nullptr);
		}

		inline FrameSyncronizationObject(unsigned int swapchainimagecount) {
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			vkCreateSemaphore(VirtualDevice::GetActive()->GetData(), &semaphoreInfo, nullptr, &imageAvailableSemaphore);

			renderFinishedSemaphores.resize(swapchainimagecount);
			for (size_t i = 0; i < swapchainimagecount; i++)
			{
				vkCreateSemaphore(VirtualDevice::GetActive()->GetData(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
			}
			
			vkCreateFence(VirtualDevice::GetActive()->GetData(), &fenceInfo, nullptr, &inFlightFence);
		}
	};
}