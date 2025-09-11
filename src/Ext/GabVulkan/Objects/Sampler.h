#pragma once

#include "../VulkanObject.h"

namespace gbe::vulkan {
    class Sampler : public VulkanObject<VkSampler, Sampler> {

    public:
        inline void RegisterDependencies() override {

        }

		inline ~Sampler(){
			vkDestroySampler(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
		}

        inline Sampler() {
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;

			//ANISOTROPY CALCULATION
			samplerInfo.maxAnisotropy = PhysicalDevice::GetActive()->Get_properties().limits.maxSamplerAnisotropy;

			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			CheckSuccess(vkCreateSampler(VirtualDevice::GetActive()->GetData(), &samplerInfo, nullptr, &this->data));

        }
    };
}