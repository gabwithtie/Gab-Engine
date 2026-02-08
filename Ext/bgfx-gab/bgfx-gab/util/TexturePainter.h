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
				static TexturePainter_bgfx implemented_instance;
			public:
				static void Init();
				TexturePainter_bgfx();
			};
		}
	}
}