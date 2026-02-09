#pragma once

// BGFX: Include the library and its platform-specific initialization header
#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include <vector>

#include "Graphics/Renderer.h"
#include "ScreenUtil.h"

#include "impl/TextureBlend.h"
#include "impl/TexturePainter.h"

namespace gbe {
	namespace gfx {
		namespace bgfx_gab {
			class bgfx_gab {
				TextureBlend_bgfx texture_blend;
				TexturePainter_bgfx texture_painter;
			public:
				inline bgfx_gab() {

				}
			};
		}
	}
}