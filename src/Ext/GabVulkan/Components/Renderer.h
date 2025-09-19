#pragma once

#include "../Objects/RenderPass.h"
#include "../Objects/Structures/AttachmentDictionary.h"
#include "../Objects/Structures/AttachmentReferencePasser.h"

namespace gbe::vulkan {
	class Renderer {
	public:
		virtual void Refresh() = 0;
		virtual RenderPass* CreateRenderPass(AttachmentDictionary& attachmentdict) = 0;
		virtual void AppendRequiredAttachments(AttachmentDictionary& attachmentdict) = 0;
		virtual void PassAttachmentReferences(AttachmentReferencePasser& attachments) = 0;
		
	};
}