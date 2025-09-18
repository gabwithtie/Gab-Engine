#pragma once

#include "../Objects/RenderPass.h"

namespace gbe::vulkan {
	class Renderer {
	public:
		virtual void Refresh() = 0;
		virtual RenderPass* CreateRenderPass() = 0;
		
	};
}