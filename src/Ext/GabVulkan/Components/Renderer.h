#pragma once

#include "../Objects/RenderPass.h"
#include "../Objects/Structures/AttachmentDictionary.h"
#include "../Objects/Structures/AttachmentReferencePasser.h"

namespace gbe::vulkan {
	class Renderer {
	protected:
		AttachmentDictionary attachments_main;
		RenderPass* main_pass = nullptr;
	public:
		inline AttachmentDictionary& GetAttachmentDictionary() {
			return attachments_main;
		}

		inline RenderPass* GetMainPass() {
			return main_pass;
		}

		virtual void Refresh(uint32_t x, uint32_t y) = 0;
		virtual void PassAttachments(AttachmentReferencePasser& passer) = 0;
	};
}