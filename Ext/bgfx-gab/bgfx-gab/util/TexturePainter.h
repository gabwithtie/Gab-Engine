#pragma once
#include <bgfx/bgfx.h>
#include <vector>
#include "Graphics/Renderer.h"

namespace gbe {
	namespace gfx {
		namespace bgfx_gab {
			class TexturePainter {
			private:
				static TexturePainter instance;

				uint16_t x;
				uint16_t y;
				uint16_t width;
				uint16_t height;
				std::vector<uint8_t> region_buffer;

				// Private members
				TextureData* target_texture = nullptr;
				Renderer* renderer = nullptr;

				// 1. Private Constructor for Singleton
				TexturePainter();

				// Add a CPU-side buffer (e.g., RGBA8)
				std::vector<uint8_t> cpu_buffer;

			public:
				// 2. Delete Copy and Assignment to prevent duplicates
				TexturePainter(const TexturePainter&) = delete;
				TexturePainter& operator=(const TexturePainter&) = delete;

				// Initialize the pointer to the renderer
				inline static void Initialize(Renderer* _renderer) {
					instance.renderer = _renderer;
				}

				static inline void SetTargetTexture(TextureData* target) {
					instance.target_texture = target;
				}

				static void Draw();

				static void Commit();
			};
		}
	}
}