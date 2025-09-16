#include "Buffer.h"

#include "Instance.h"

namespace gbe::vulkan {
	Buffer::~Buffer() {
        if (frame_index == -1) {
            vkDestroyBuffer(VirtualDevice::GetActive()->GetData(), this->data, nullptr);
            vkFreeMemory(VirtualDevice::GetActive()->GetData(), bufferMemory, nullptr);
        }
        else {
            Instance::GetActive()->QueueBufferDeletion(new Buffer(*this));
        }
	}
}