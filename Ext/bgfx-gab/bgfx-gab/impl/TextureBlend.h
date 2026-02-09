#pragma once
#include <bgfx/bgfx.h>
#include <vector>
#include "Graphics/Renderer.h"
#include "Graphics/util/TextureBlend.h"

namespace gbe {
	namespace gfx {
		namespace bgfx_gab {
			class TextureBlend_bgfx : public TextureBlend {
			public:
				TextureBlend_bgfx();
			};
		}
	}
}