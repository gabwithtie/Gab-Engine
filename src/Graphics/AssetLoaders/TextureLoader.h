#pragma once
#include "Ext/GabVulkan/Objects.h"
#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"
#include <stb_image.h>
#include <functional>
#include <unordered_map>
#include <stack>

namespace gbe {
	namespace gfx {		
		struct TextureData {
			vulkan::ImageView* textureImageView;
			vulkan::Image* textureImage;
			vulkan::Sampler* textureSampler;

			unsigned int width;
			unsigned int height;

			//UI
			bool gui_initialized = false;
			VkDescriptorSet DS;
		};

		typedef std::function<VkDescriptorSet(gbe::vulkan::Sampler*, gbe::vulkan::ImageView*)> GbeUiCallbackFunction;

		class TextureLoader : public asset::AssetLoader<asset::Texture, asset::data::TextureImportData, asset::data::TextureLoadData, TextureData> {
		private:
			TextureData defaultImage;
			static GbeUiCallbackFunction Ui_Callback;
		protected:
			TextureData LoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) override;
			void UnLoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) override;
		public:
			void AssignSelfAsLoader() override;
			static TextureData& GetDefaultImage();
			inline static VkDescriptorSet GetGuiHandle(TextureData* data) {
				if (data->gui_initialized)
					return data->DS;

				data->DS = Ui_Callback(data->textureSampler, data->textureImageView);
				data->gui_initialized = true;

				return data->DS;
			}
			const static void Set_Ui_Callback(GbeUiCallbackFunction func);
		};
	}
}