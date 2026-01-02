#include "TextureLoader.h"
#include "../RenderPipeline.h"

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <stdexcept>

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

    // 1. Read file into memory (same as before)
    bx::FileReader reader;
    bx::open(&reader, pathstr.string().c_str());
    uint32_t size = (uint32_t)bx::getSize(&reader);
    uint8_t* raw_data = (uint8_t*)bx::alloc(&s_allocator, size);
    bx::Error read_err;
    bx::read(&reader, raw_data, size, &read_err);
    bx::close(&reader);

    // 2. Parse the header to get dimensions
    bx::Error err;
    bimg::ImageContainer header;
    bimg::imageParse(header, static_cast<bx::ReaderSeekerI*>(&reader), & err);
    if (!err.isOk()) {
        bx::free(&s_allocator, raw_data);
        throw std::runtime_error("Failed to parse texture header");
    }

    uint32_t width = header.m_width;
    uint32_t height = header.m_height;
    uint32_t depth = header.m_depth;

    // 3. Allocate memory for the 32-bit Float RGBA output
    // (Width * Height * 4 channels * 4 bytes per float)
    uint32_t dst_pitch = width * 16;
    uint32_t dest_size = width * height * 16;
    float* float_pixels = (float*)bx::alloc(&s_allocator, dest_size);

    // 4. Decode specifically to RGBA32F
    // Note the signature: (allocator, dest, src, src_size, error)
    bimg::imageDecodeToRgba32f(&s_allocator, float_pixels, raw_data, width, height, depth, dst_pitch, header.m_format);

    // Clean up header and raw file data immediately
    bimg::imageFree(&header);
    bx::free(&s_allocator, raw_data);

    // 5. Create the bgfx Texture
    // IMPORTANT: You MUST use bgfx::TextureFormat::RGBA32F
    const bgfx::Memory* mem = bgfx::makeRef(float_pixels, dest_size, [](void* ptr, void* userData) {
        bx::free(&s_allocator, ptr); // Custom free to clean up the float buffer
        }, nullptr);

    bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
        (uint16_t)width,
        (uint16_t)height,
        false, 1,
        bgfx::TextureFormat::RGBA32F,
        BGFX_TEXTURE_NONE,
        mem
    );

    return TextureData{
        .textureHandle = textureHandle,
        .width = width,
        .height = height
    };
}

void gbe::gfx::TextureLoader::UnLoadAsset_(asset::Texture* asset, const asset::data::TextureImportData& importdata, asset::data::TextureLoadData* data)
{
    const auto& texturedata = this->GetAssetData(asset);
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