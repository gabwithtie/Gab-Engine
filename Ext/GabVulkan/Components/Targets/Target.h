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
		std::vector<Framebuffer*> framebuffers;

		bool started = false;
	public:
		inline bool Get_ifstarted() {
			return started;
		}

		inline ~Target() {
			delete renderpass;

			auto framecount = framebuffers.size();
			for (size_t i = 0; i < framecount; i++)
			{
				delete framebuffers[i];
			}
		}

		inline virtual void StartPass(uint32_t drawlayer = 0) {
			started = true;
		};
		inline virtual void EndPass() {
			started = false;
		};
	};
}