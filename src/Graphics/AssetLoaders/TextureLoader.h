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
			vulkan::ImageView textureImageView;
			vulkan::Image textureImage;
			vulkan::Sampler textureSampler;

			VkDescriptorSet DS;

			unsigned int width;
			unsigned int height;
		};

		class TextureLoader : public asset::AssetLoader<asset::Texture, asset::data::TextureImportData, asset::data::TextureLoadData, TextureData> {
		private:
			VkDevice* vkdevice;
			VkPhysicalDevice* vkphysicaldevice;
			TextureData defaultImage;

			static std::function<VkDescriptorSet(gbe::vulkan::Sampler, gbe::vulkan::ImageView)> Ui_Callback;
		protected:
			TextureData LoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) override;
			void UnLoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) override;
		public:
			static TextureData& GetDefaultImage();
			const static void Set_Ui_Callback(std::function<VkDescriptorSet(gbe::vulkan::Sampler, gbe::vulkan::ImageView)> func);
			void PassDependencies(VkDevice* vkdevice, VkPhysicalDevice* vkphysicaldevice);
		};
	}
}