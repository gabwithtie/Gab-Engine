#pragma once

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include <vector>

namespace gbe {
	namespace gfx {
		namespace bgfx_gab {

			// --- Helper for Screen Space Passes ---
			struct ScreenVertex {
				float x, y, z;
				float u, v;
				static bgfx::VertexLayout ms_layout;
			};

			extern void RenderFullscreenPass(bgfx::ViewId _view, bgfx::ProgramHandle _program);

		}
	}
}