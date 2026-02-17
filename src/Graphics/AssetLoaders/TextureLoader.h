#pragma once
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
			bgfx::TextureFormat::Enum format; // <--- Add this
			uint32_t bitsPerPixel;

			// Vulkan objects removed:
			// vulkan::ImageView* textureImageView;
			// vulkan::Image* textureImage;
			// vulkan::Sampler* textureSampler;
			std::vector<uint8_t> data;

			//UI
			bool gui_initialized = false;
			// VkDescriptorSet DS; // REMOVED

			Vector2Int dimensions;
			int colorchannels;
		};

		// typedef std::function<VkDescriptorSet(gbe::vulkan::Sampler*, gbe::vulkan::ImageView*)> GbeUiCallbackFunction; // REMOVED

		class TextureLoader : public asset::AssetLoader<asset::Texture, asset::data::TextureImportData, TextureData> {
		private:
			TextureData defaultImage;
			// static GbeUiCallbackFunction Ui_Callback; // REMOVED
		protected:
			void LoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, TextureData* data) override;
			void UnLoadAsset_(TextureData* data) override;
		public:
			void AssignSelfAsLoader() override;
			static TextureData& GetDefaultImage();
			static void ReSave(asset::Texture* asset);

			inline virtual void OnAsyncTaskCompleted(AsyncLoadTask* loadtask) override {
				//This is a synchronous loader
			}
		};
	}
}