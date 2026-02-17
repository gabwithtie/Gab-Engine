#pragma once
#include <vector>
#include "Graphics/Renderer.h"

#include <functional>

namespace gbe {
	namespace gfx {
		class TexturePainter {
		protected:
			static TexturePainter* instance;
			static bool enabled;

			asset::Texture* target_texture = nullptr;

			// Add a CPU-side buffer (e.g., RGBA8)
			std::vector<uint8_t> m_localBuffer;

			std::function<void()> impl_change_tex;
			std::function<void(Vector2Int pos)> impl_draw;

			int brush_size = 5;
			float brush_strength = 5;
			Vector4 brush_color = Vector4(1.0f, 0.0f, 0.0f, 1.0f); // Red by default
		public:
			static inline void ApplyAndSave() {
				if (instance->target_texture == nullptr)
					return;

				auto data = TextureLoader::GetAssetRuntimeData(instance->target_texture->Get_assetId());
				data->data = instance->m_localBuffer; // Update CPU-side data

				TextureLoader::ReSave(instance->target_texture); // Save to disk (and GPU) using TextureLoader's existing logic
			}
			static inline void SetBrushSize(int size) {
				instance->brush_size = size;
			}
			static inline int GetBrushSize() {
				return instance->brush_size;
			}
			static inline void SetBrushStrength(float strength) {
				instance->brush_strength = strength;
			}
			static inline float GetBrushStrength() {
				return instance->brush_strength;
			}
			static inline void SetBrushColor(Vector4 color) {
				instance->brush_color = color;
			}
			static inline Vector4 GetBrushColor() {
				return instance->brush_color;
			}

			static inline bool GetEnabled() {
				return enabled;
			}

			static void SetEnabled(bool en);

			static inline void SetTargetTexture(asset::Texture* target) {
				instance->target_texture = target;
				instance->impl_change_tex();
			}
			static inline asset::Texture* GetTargetTexture() {
				return instance->target_texture;
			}

			static void Draw(Vector2Int pos);
		};
	}
}