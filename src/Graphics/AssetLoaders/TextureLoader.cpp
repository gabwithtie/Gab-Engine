#include "TextureLoader.h"
#include "../RenderPipeline.h"

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <stdexcept>

#include <bgfx_utils.h>

// Global or static allocator for bimg
static bx::DefaultAllocator s_allocator;

// Helper to release bimg container memory once bgfx is done with it
static void imageReleaseCallback(void* _ptr, void* _userData) {
    bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
    bimg::imageFree(imageContainer);
}

gbe::gfx::TextureData gbe::gfx::TextureLoader::LoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* loaddata) {
    const auto& pathstr = target->Get_asset_filepath().parent_path() / importdata.filename;
    std::string path = pathstr.string();

    bimg::ImageContainer* imageContainer = imageLoad(pathstr.string().c_str(), bgfx::TextureFormat::Count);

    if (imageContainer == nullptr) {
        throw std::runtime_error("Failed to decode texture: " + path);
    }

    loaddata->dimensions = Vector2Int(imageContainer->m_width, imageContainer->m_height);

    // 3. Create bgfx Texture
    uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;

    // Use makeRef with a callback so we don't have to copy the decoded memory again.
    // bgfx will call imageReleaseCallback when it no longer needs the pointer.
    const bgfx::Memory* mem = bgfx::makeRef(
        imageContainer->m_data,
        imageContainer->m_size,
        imageReleaseCallback,
        imageContainer
    );

    bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
        (uint16_t)imageContainer->m_width,
        (uint16_t)imageContainer->m_height,
        false, 1,
        (bgfx::TextureFormat::Enum)imageContainer->m_format,
        flags,
        mem
    );

    // Always check if the handle is valid
    if (!bgfx::isValid(textureHandle)) {
        throw std::runtime_error("bgfx failed to create texture: Invalid parameters or unsupported format.");
    }

    return TextureData{
        .textureHandle = textureHandle,
        .width = (uint16_t)imageContainer->m_width,
        .height = (uint16_t)imageContainer->m_height
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