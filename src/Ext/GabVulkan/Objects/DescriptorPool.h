#pragma once

#include "../VulkanObject.h"
#include "../VulkanObjectSingleton.h"

namespace gbe::vulkan {
    class DescriptorPool : public VulkanObject<VkDescriptorPool, DescriptorPool> {
    protected:
    public:
        inline void RegisterDependencies() override {

        }

        inline DescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags = 0) {
            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.flags = flags;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = maxSets;

            CheckSuccess(vkCreateDescriptorPool(VirtualDevice::GetActive()->GetData(), &poolInfo, nullptr, &this->data));
        }
    };
}