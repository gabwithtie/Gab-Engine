#pragma once
#include <bgfx/bgfx.h>
#include <vector>
#include "Graphics/Renderer.h"

#include <functional>

namespace gbe {
	namespace gfx {
		class TexturePainter {
		public:
			struct DrawCommand {
				Vector2Int pixel_screen_position;
				uint16_t x;
				uint16_t y;
				uint16_t width;
				uint16_t height;
				std::vector<uint8_t> cpu_buffer;
			};

		protected:
			static TexturePainter* instance;
			static bool enabled;

			std::vector<DrawCommand> draw_queue;
			TextureData* target_texture = nullptr;

			// Add a CPU-side buffer (e.g., RGBA8)
			std::vector<uint8_t> cpu_buffer;

			std::function<void(Vector2Int pos)> draw_callback;
			std::function<void(const DrawCommand&)> commit_callback;
		public:
			static inline bool GetEnabled() {
				return enabled;
			}

			static void SetEnabled(bool en);

			static inline void SetTargetTexture(TextureData* target) {
				instance->target_texture = target;
			}
			static void Draw(Vector2Int pos);
			static void Commit();
		};
	}
}