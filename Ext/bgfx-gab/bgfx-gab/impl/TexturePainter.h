#pragma once
#include <bgfx/bgfx.h>
#include <vector>
#include "Graphics/Renderer.h"
#include "Graphics/util/TexturePainter.h"

namespace gbe {
	namespace gfx {
		namespace bgfx_gab {
			class TexturePainter_bgfx : public TexturePainter {
			private:
				bgfx::TextureInfo m_info;
			public:
				TexturePainter_bgfx();
			};
		}
	}
}