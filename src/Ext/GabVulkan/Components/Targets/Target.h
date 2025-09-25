#pragma once

#include "../../Objects/Framebuffer.h"
#include "../../Objects/RenderPass.h"

#include "../../Objects/Structures/AttachmentDictionary.h"

namespace gbe::vulkan {
	class Target {
	protected:
		uint32_t x;
		uint32_t y;

		RenderPass* renderpass;
		Framebuffer* framebuffer;
	public:
		inline ~Target() {
			delete renderpass;
			delete framebuffer;
		}

		virtual void StartPass() = 0;
		virtual void EndPass() = 0;
	};
}