#include "TextureLoader.h"

#include "../RenderPipeline.h"

//Function is to be injected by the UI library in use
gbe::gfx::GbeUiCallbackFunction gbe::gfx::TextureLoader::Ui_Callback;

gbe::gfx::TextureData gbe::gfx::TextureLoader::LoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* loaddata) {
	const auto& pathstr = target->Get_asset_filepath().parent_path() / importdata.filename;

	stbi_uc* pixels;
	int tex_width;
	int tex_height;
	int colorchannels;

	stbi_set_flip_vertically_on_load(true);
	pixels = stbi_load(pathstr.string().c_str(), &tex_width, &tex_height, &colorchannels, 4);

	if(pixels == nullptr) {
		throw std::runtime_error("Failed to load texture image!");
	}

	//GET PIXEL COLORS HERE

	loaddata->colorchannels = colorchannels;
	loaddata->dimensions = Vector2Int(tex_width, tex_height);

	//VULKAN TEXTURE LOAD
	VkDeviceSize imageSizevk = tex_width * tex_height * 4;

	vulkan::Buffer stagingBuffer(imageSizevk, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* vkdata;
	vulkan::VirtualDevice::GetActive()->MapMemory(stagingBuffer.GetMemory(), 0, imageSizevk, 0, &vkdata);
	memcpy(vkdata, pixels, static_cast<size_t>(imageSizevk));
	vulkan::VirtualDevice::GetActive()->UnMapMemory(stagingBuffer.GetMemory());

	stbi_image_free(pixels);
	
	//MM_note: will be freed by Unload asset function.

	vulkan::Image* textureImage = new vulkan::Image(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vulkan::ImageView* textureImageView = new vulkan::ImageView(textureImage, VK_IMAGE_ASPECT_COLOR_BIT);
	vulkan::Sampler* textureSampler = new vulkan::Sampler();

	//DO UI LOADING HERE
	VkDescriptorSet tex_DS = nullptr;
	bool ui_initted = false;

	if (importdata.type == "UI") {
		if (TextureLoader::Ui_Callback)
		{
			tex_DS = TextureLoader::Ui_Callback(textureSampler, textureImageView);
			ui_initted = true;
		}
		else {
			std::cerr << "Texture loaded as a UI Texture but UI Texture Loader not initialized." << std::endl;
		}
	}

	//Map memory
	textureImage->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vulkan::Image::copyBufferToImage(&stagingBuffer, textureImage);
	textureImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return TextureData{
		.textureImageView = textureImageView,
		.textureImage = textureImage,
		.textureSampler = textureSampler,
		.gui_initialized = ui_initted,
		.DS = tex_DS
	};
}
void gbe::gfx::TextureLoader::UnLoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) {
	const auto& assetdata = GetAssetData(target);

	delete assetdata.textureImage;
	delete assetdata.textureImageView;
	delete assetdata.textureSampler;
}

gbe::gfx::TextureData& gbe::gfx::TextureLoader::GetDefaultImage()
{
	return static_cast<TextureLoader*>(active_instance)->defaultImage;
}

void gbe::gfx::TextureLoader::AssignSelfAsLoader() {
	asset::AssetLoader<asset::Texture, asset::data::TextureImportData, asset::data::TextureLoadData, TextureData>::AssignSelfAsLoader();

	//CREATING DEFAULT IMAGE
	int tex_width = 1;
	int tex_height = 1;
	int colorchannels = 3;

	stbi_uc* pixels = (stbi_uc*)calloc(tex_width * tex_height * colorchannels, 1);

	//VULKAN TEXTURE LOAD
	VkDeviceSize imageSize = tex_width * tex_height * (colorchannels + 1);

	vulkan::Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* vkdata;
	vulkan::VirtualDevice::GetActive()->MapMemory(stagingBuffer.GetMemory(), 0, imageSize, 0, &vkdata);
	memcpy(vkdata, pixels, static_cast<size_t>(imageSize));
	vulkan::VirtualDevice::GetActive()->UnMapMemory(stagingBuffer.GetMemory());

	stbi_image_free(pixels);

	vulkan::Image* textureImage = new vulkan::Image(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vulkan::ImageView* textureImageView = new vulkan::ImageView(textureImage, VK_IMAGE_ASPECT_COLOR_BIT);
	vulkan::Sampler* textureSampler = new vulkan::Sampler();

	textureImage->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vulkan::Image::copyBufferToImage(&stagingBuffer, textureImage);
	textureImage->transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	this->defaultImage = TextureData{
		textureImageView,
		textureImage,
		textureSampler
	};
}

const void gbe::gfx::TextureLoader::Set_Ui_Callback(GbeUiCallbackFunction func) {
	gbe::gfx::TextureLoader::Ui_Callback = func;
}
