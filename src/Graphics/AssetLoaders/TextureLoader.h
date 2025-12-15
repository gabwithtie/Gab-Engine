#pragma once
// #include "Ext/GabVulkan/Objects.h" // REMOVED
#include <bgfx/bgfx.h> // ADDED bgfx
#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"
#include <functional>
#include <unordered_map>
#include <stack>

namespace gbe {
	namespace gfx {
		struct TextureData {
			// Replaced Vulkan objects with bgfx handle
			bgfx::TextureHandle textureHandle = BGFX_INVALID_HANDLE;

			// Vulkan objects removed:
			// vulkan::ImageView* textureImageView;
			// vulkan::Image* textureImage;
			// vulkan::Sampler* textureSampler;

			unsigned int width;
			unsigned int height;

			//UI
			bool gui_initialized = false;
			// VkDescriptorSet DS; // REMOVED
		};

		// typedef std::function<VkDescriptorSet(gbe::vulkan::Sampler*, gbe::vulkan::ImageView*)> GbeUiCallbackFunction; // REMOVED

		class TextureLoader : public asset::AssetLoader<asset::Texture, asset::data::TextureImportData, asset::data::TextureLoadData, TextureData> {
		private:
			TextureData defaultImage;
			// static GbeUiCallbackFunction Ui_Callback; // REMOVED
		protected:
			TextureData LoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) override;
			void UnLoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) override;
		public:
			void AssignSelfAsLoader() override;
			static TextureData& GetDefaultImage();

			// GetGuiHandle now returns the bgfx handle. The UI layer (e.g., ImGui's bgfx backend) uses this directly.
			inline static bgfx::TextureHandle GetGuiHandle(TextureData* data) {
				if (data->gui_initialized == false) {
					// In a full implementation, you might register this handle with the UI system here.
					data->gui_initialized = true;
				}
				return data->textureHandle;
			}
		};
	}
}