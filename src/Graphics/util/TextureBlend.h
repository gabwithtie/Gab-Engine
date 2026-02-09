#pragma once
#include <vector>
#include "Graphics/Renderer.h"

#include <functional>

namespace gbe {
	namespace gfx {
		class TextureBlend {
		public:
			enum BlendMode {
				BLEND_ALPHA,
				BLEND_ADDITIVE,
				BLEND_MULTIPLY,
			};
		protected:
			static TextureBlend* instance;

			std::function<void(TextureData*, TextureData*, TextureData*, BlendMode)> impl_blend;
		public:
			static inline void BlendTextures(TextureData* a, TextureData* b, TextureData* dest, BlendMode mode) {
				if (TextureBlend::instance && TextureBlend::instance->impl_blend) {
					TextureBlend::instance->impl_blend(a, b, dest, mode);
				}
			}
		};
	}
}