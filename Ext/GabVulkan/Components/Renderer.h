#pragma once

#include "../Objects/RenderPass.h"
#include "../Objects/Structures/AttachmentDictionary.h"
#include "../Objects/Structures/AttachmentReferencePasser.h"

namespace gbe::vulkan {
	class Renderer {
	protected:
		AttachmentDictionary attachments_screen;
		RenderPass* screen_pass = nullptr;
	public:
		inline AttachmentDictionary& GetAttachmentDictionary() {
			return attachments_screen;
		}

		inline RenderPass* GetScreenPass() {
			return screen_pass;
		}

		virtual void Refresh(uint32_t x, uint32_t y) = 0;
		virtual void PassAttachments(AttachmentReferencePasser& passer) = 0;
	};
}