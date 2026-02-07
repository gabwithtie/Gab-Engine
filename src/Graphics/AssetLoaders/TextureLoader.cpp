#include "TextureLoader.h"
#include "../RenderPipeline.h"

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <stdexcept>
#include <bimg/bimg.h>

#include <bgfx_utils.h>

// Global or static allocator for bimg
static bx::DefaultAllocator s_allocator;

// Helper to release bimg container memory once bgfx is done with it
static void imageReleaseCallback(void* _ptr, void* _userData) {
    bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
    bimg::imageFree(imageContainer);
}

gbe::gfx::TextureData gbe::gfx::TextureLoader::LoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* loaddata) {
    if (importdata.path.size() == 0)
        return {};

    const auto& pathstr = target->Get_asset_filepath().parent_path() / importdata.path;
    bimg::ImageContainer* imageContainer = imageLoad(pathstr.string().c_str(), bgfx::TextureFormat::Count);

    if (imageContainer == nullptr) {
        throw std::runtime_error("Failed to decode texture: " + pathstr.string());
    }

    loaddata->dimensions = Vector2Int(imageContainer->m_width, imageContainer->m_height);

    // 1. Correct Flags for a Mutable Texture
    // BGFX_TEXTURE_NONE is the base. 
    // BGFX_TEXTURE_BLIT_DST allows it to be a destination for blits/updates.
    uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_TEXTURE_BLIT_DST;

    // 2. Create the Texture WITHOUT initial data (pass nullptr)
    // This creates a dynamic resource that can be updated later.
    auto import_format = (bgfx::TextureFormat::Enum)imageContainer->m_format;

    bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
        (uint16_t)imageContainer->m_width,
        (uint16_t)imageContainer->m_height,
        false, 1,
        import_format,
        flags,
        nullptr // Pass nullptr here to make it mutable
    );

    if (!bgfx::isValid(textureHandle)) {
        bimg::imageFree(imageContainer);
        throw std::runtime_error("bgfx failed to create texture.");
    }

    // 3. Upload the initial image data immediately using updateTexture2D
    // We use bgfx::copy here because imageLoad's memory is managed by imageContainer
    bgfx::updateTexture2D(
        textureHandle,
        0, 0,
        0, 0,
        (uint16_t)imageContainer->m_width,
        (uint16_t)imageContainer->m_height,
        bgfx::makeRef(imageContainer->m_data, imageContainer->m_size, imageReleaseCallback, imageContainer)
    );

    return TextureData{
        .textureHandle = textureHandle,
		.format = import_format,
        .bitsPerPixel = bimg::getBitsPerPixel((bimg::TextureFormat::Enum)import_format),
        .width = (uint32_t)imageContainer->m_width,
        .height = (uint32_t)imageContainer->m_height,
    };
}

void gbe::gfx::TextureLoader::UnLoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data)
{
    const auto& texturedata = this->GetAssetRuntimeData(asset->Get_assetId());
    if (bgfx::isValid(texturedata.textureHandle)) {
        bgfx::destroy(texturedata.textureHandle);
    }
}

void gbe::gfx::TextureLoader::AssignSelfAsLoader()
{
    AssetLoader::AssignSelfAsLoader();

    // Default 1x1 White Pixel
    const uint32_t width = 1;
    const uint32_t height = 1;
    const bgfx::Memory* mem = bgfx::alloc(width * height * 4);
    uint32_t* pixels = (uint32_t*)mem->data;
    pixels[0] = 0xffffffff;

    bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
        (uint16_t)width,
        (uint16_t)height,
        false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem
    );

    defaultImage = TextureData{
        .textureHandle = textureHandle,
        .width = width,
        .height = height
    };
}

gbe::gfx::TextureData& gbe::gfx::TextureLoader::GetDefaultImage() {
    return static_cast<gbe::gfx::TextureLoader*>(active_base_instance)->defaultImage;
}