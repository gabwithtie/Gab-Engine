#include "TextureLoader.h"

#include "../RenderPipeline.h"

//Function is to be injected by the UI library in use
std::function<VkDescriptorSet(gbe::vulkan::Sampler, gbe::vulkan::ImageView)> gbe::gfx::TextureLoader::Ui_Callback;

gbe::gfx::TextureData gbe::gfx::TextureLoader::LoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* loaddata) {
	std::string pathstr = target->Get_asset_directory() + importdata.filename;

	stbi_uc* pixels;
	int tex_width;
	int tex_height;
	int colorchannels;

	stbi_set_flip_vertically_on_load(true);
	pixels = stbi_load(pathstr.c_str(), &tex_width, &tex_height, &colorchannels, 4);

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

	//IMAGE CREATION

	vulkan::Image textureImage(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//CREATE IMAGE VIEW
	vulkan::ImageView textureImageView(textureImage, VK_IMAGE_ASPECT_COLOR_BIT);

	//SAMPLING
	vulkan::Sampler textureSampler;

	//DO UI LOADING HERE
	VkDescriptorSet tex_DS = nullptr;

	if(importdata.type == "UI")
		tex_DS = TextureLoader::Ui_Callback(textureSampler, textureImageView);


	//Map memory
	textureImage.transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vulkan::Image::copyBufferToImage(stagingBuffer, textureImage);

	textureImage.transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return TextureData{
		textureImageView,
		textureImage,
		textureSampler,
		tex_DS
	};
}
void gbe::gfx::TextureLoader::UnLoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data) {

}

gbe::gfx::TextureData& gbe::gfx::TextureLoader::GetDefaultImage()
{
	return static_cast<TextureLoader*>(active_instance)->defaultImage;
}

const void gbe::gfx::TextureLoader::Set_Ui_Callback(std::function<VkDescriptorSet(gbe::vulkan::Sampler, gbe::vulkan::ImageView)> func) {
	gbe::gfx::TextureLoader::Ui_Callback = func;
}

void gbe::gfx::TextureLoader::PassDependencies(VkDevice* vkdevice, VkPhysicalDevice* vkphysicaldevice)
{
	this->vkdevice = vkdevice;
	this->vkphysicaldevice = vkphysicaldevice;

	//CREATING DEFAULT IMAGE
	//IMAGE CREATION

	int tex_width = 1;
	int tex_height = 1;
	int colorchannels = 3;

	stbi_uc* pixels = (stbi_uc*)calloc(tex_width * tex_height * colorchannels, 1);

	//VULKAN TEXTURE LOAD
	VkDeviceSize imageSize = tex_width * tex_height * (colorchannels + 1);

	vulkan::Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* vkdata;
	vkMapMemory(*this->vkdevice, stagingBuffer.GetMemory(), 0, imageSize, 0, &vkdata);
	memcpy(vkdata, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(*this->vkdevice, stagingBuffer.GetMemory());

	stbi_image_free(pixels);

	vulkan::Image textureImage(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	textureImage.transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vulkan::Image::copyBufferToImage(stagingBuffer, textureImage);

	textureImage.transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//CREATE IMAGE VIEW
	vulkan::ImageView textureImageView(textureImage, VK_IMAGE_ASPECT_COLOR_BIT);

	//SAMPLING
	vulkan::Sampler textureSampler;

	this->defaultImage = TextureData{
		textureImageView,
		textureImage,
		textureSampler
	};
}
