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

		virtual void Refresh() = 0;
	};
}