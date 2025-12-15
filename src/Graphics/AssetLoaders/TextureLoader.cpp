#include "TextureLoader.h"

#include <stb_image.h>

#include "../RenderPipeline.h"
#include <stdexcept>

// BGFX: Helper function to map channels to bgfx format (using RGBA8 / 4 channels)
bgfx::TextureFormat::Enum GetBgfxFormat(int colorchannels) {
	// Assuming 4 channels (RGBA) are always loaded for consistency
	return bgfx::TextureFormat::RGBA8;
}


gbe::gfx::TextureData gbe::gfx::TextureLoader::LoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* loaddata) {
	const auto& pathstr = target->Get_asset_filepath().parent_path() / importdata.filename;

	stbi_uc* pixels;
	int tex_width;
	int tex_height;
	int colorchannels;

	// Force 4 channels (RGBA) for consistent GPU loading
	stbi_set_flip_vertically_on_load(true);
	pixels = stbi_load(pathstr.string().c_str(), &tex_width, &tex_height, &colorchannels, 4);

	if (pixels == nullptr) {
		throw std::runtime_error("Failed to load texture image!");
	}

	loaddata->colorchannels = colorchannels;
	loaddata->dimensions = Vector2Int(tex_width, tex_height);

	//===================BGFX TEXTURE LOAD===================

	bgfx::TextureFormat::Enum format = GetBgfxFormat(4);

	// 1. Create a memory buffer for bgfx from the loaded pixels
	const bgfx::Memory* mem = bgfx::copy(pixels, tex_width * tex_height * 4);

	// 2. Create the texture handle
	uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;

	// Assuming a property for SRGB might be available
	//if (importdata.srgb_enabled)
	{
		flags |= BGFX_TEXTURE_SRGB;
	}

	bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
		(uint16_t)tex_width,
		(uint16_t)tex_height,
		false, // Not using mips generated from the original data
		1,     // Number of layers/array slices
		format,
		flags,
		mem
	);

	// 3. Free CPU memory
	stbi_image_free(pixels);

	if (textureHandle.idx == bgfx::kInvalidHandle) {
		throw std::runtime_error("Failed to create bgfx texture handle!");
	}


	//COMMITTING
	return TextureData{
		.textureHandle = textureHandle,
		.width = (unsigned int)tex_width,
		.height = (unsigned int)tex_height
	};
}


void gbe::gfx::TextureLoader::UnLoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data)
{
	const auto& texturedata = this->GetAssetData(asset);

	// BGFX: Destroy the handle
	if (bgfx::isValid(texturedata.textureHandle)) {
		bgfx::destroy(texturedata.textureHandle);
	}
}

void gbe::gfx::TextureLoader::AssignSelfAsLoader()
{
	// Load the default image (1x1 white pixel)
	int tex_width = 1;
	int tex_height = 1;
	int colorchannels = 4; // Force RGBA

	// Allocate and set the pixel to white
	stbi_uc* pixels = (stbi_uc*)calloc(tex_width * tex_height * colorchannels, 1);
	pixels[0] = 255; // R
	pixels[1] = 255; // G
	pixels[2] = 255; // B
	pixels[3] = 255; // A

	//===================BGFX DEFAULT TEXTURE LOAD===================
	bgfx::TextureFormat::Enum format = GetBgfxFormat(4);
	const bgfx::Memory* mem = bgfx::copy(pixels, tex_width * tex_height * 4);

	bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
		(uint16_t)tex_width,
		(uint16_t)tex_height,
		false,
		1,
		format,
		BGFX_TEXTURE_NONE,
		mem
	);

	free(pixels);

	if (textureHandle.idx == bgfx::kInvalidHandle) {
		throw std::runtime_error("Failed to create bgfx default texture handle!");
	}

	// COMMITTING
	defaultImage = TextureData{
		.textureHandle = textureHandle,
		.width = (unsigned int)tex_width,
		.height = (unsigned int)tex_height
	};
}

gbe::gfx::TextureData& gbe::gfx::TextureLoader::GetDefaultImage()
{
	return static_cast<gbe::gfx::TextureLoader*>(active_base_instance)->defaultImage;
}