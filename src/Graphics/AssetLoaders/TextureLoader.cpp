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

void gbe::gfx::TextureLoader::ReSave(asset::Texture* asset)
{
	auto data = GetAssetRuntimeData(asset->Get_assetId());

    if (data->data.empty()) return;

    // 1. Open a file writer using bx (bgfx's companion library)
    bx::FileWriter writer;
    bx::Error err;

	auto folderpath = asset->Get_asset_filepath().parent_path();
	auto fullpath = folderpath / asset->Get_import_data().path;

    if (bx::open(&writer, fullpath.string().c_str())) {
        
        uint32_t bpp = data->bitsPerPixel / 8;
        uint32_t srcPitch = data->dimensions.x * bpp;

        // 4. Call imageWritePng
        bimg::imageWritePng(
            &writer,            // _writer: The open bx::FileWriter
            data->dimensions.x,       // _width
            data->dimensions.y,      // _height
            srcPitch,           // _srcPitch: Row size in bytes
            data->data.data(), // _src: Pointer to your raw CPU buffer
            bimg::TextureFormat::RGBA8, // _format: The format of your CPU buffer
            false,              // _yflip: Set true if the image comes out upside down
            &err                // _err: Pointer to error object
        );

        if (!err.isOk()) {
            // Handle encoding error (e.g., out of memory, disk full)
            printf("PNG Encoding Error: %s\n", err.getMessage().getCPtr());
        }

        bx::close(&writer);
    }
}

void gbe::gfx::TextureLoader::LoadAsset_(gbe::asset::Texture* target, const asset::data::TextureImportData& importdata, TextureData* loaddata) {
    if (importdata.path.size() == 0) return;

    const auto& pathstr = target->Get_asset_filepath().parent_path() / importdata.path;
    bimg::ImageContainer* imageContainer = imageLoad(pathstr.string().c_str(), bgfx::TextureFormat::Count);

    if (imageContainer == nullptr) {
        throw std::runtime_error("Failed to decode texture: " + pathstr.string());
    }

    uint32_t width = imageContainer->m_width;
    uint32_t height = imageContainer->m_height;
    loaddata->dimensions = Vector2Int(width, height);

    bgfx::TextureFormat::Enum nativeFormat = (bgfx::TextureFormat::Enum)imageContainer->m_format;
    bgfx::TextureFormat::Enum targetFormat = bgfx::TextureFormat::RGBA8;

    std::vector<uint8_t> finalData;
    uint32_t expectedSize = width * height * 4; // 4 bytes per pixel for RGBA8
    finalData.resize(expectedSize);

    if (nativeFormat == bgfx::TextureFormat::RGBA16) {
        const uint16_t* src16 = reinterpret_cast<const uint16_t*>(imageContainer->m_data);
        uint8_t* dst8 = finalData.data();
        for (size_t i = 0; i < (size_t)width * height * 4; ++i) {
            dst8[i] = static_cast<uint8_t>(src16[i] >> 8);
        }
    }
    else if (nativeFormat == bgfx::TextureFormat::RGB8) {
        const uint8_t* srcRGB = reinterpret_cast<const uint8_t*>(imageContainer->m_data);
        uint8_t* dstRGBA = finalData.data();
        for (size_t i = 0; i < (size_t)width * height; ++i) {
            dstRGBA[i * 4 + 0] = srcRGB[i * 3 + 0];
            dstRGBA[i * 4 + 1] = srcRGB[i * 3 + 1];
            dstRGBA[i * 4 + 2] = srcRGB[i * 3 + 2];
            dstRGBA[i * 4 + 3] = 255;
        }
    }
    else if (nativeFormat == bgfx::TextureFormat::RGBA8) {
        memcpy(finalData.data(), imageContainer->m_data, expectedSize);
    }
    else {
        // --- USING YOUR SPECIFIC imageConvert SIGNATURE ---

        // 1. Get the unpack function for the source (native) format
        bimg::UnpackFn unpack = bimg::getUnpack((bimg::TextureFormat::Enum)nativeFormat);

        // 2. Get the pack function for our target (RGBA8)
        bimg::PackFn pack = bimg::getPack(bimg::TextureFormat::RGBA8);

        if (unpack && pack) {
            // _bpp is 32 (bits per pixel) for RGBA8
            // _size is usually the pixel count (width * height) for this signature
            bimg::imageConvert(
                finalData.data(),           // _dst
                32,                         // _bpp
                pack,                       // _pack
                imageContainer->m_data,     // _src
                unpack,                     // _unpack
                width * height              // _size (pixel count)
            );
        }
        else {
            // Fallback or Error if format is unsupported by bimg's internal table
            bimg::imageFree(imageContainer);
            throw std::runtime_error("Unsupported texture format for conversion.");
        }
    }

    // --- GPU RESOURCE CREATION ---

    uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_TEXTURE_BLIT_DST;

    bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
        (uint16_t)width, (uint16_t)height, false, 1,
        targetFormat, flags, nullptr
    );

    if (!bgfx::isValid(textureHandle)) {
        bimg::imageFree(imageContainer);
        throw std::runtime_error("bgfx failed to create texture.");
    }

    bgfx::updateTexture2D(
        textureHandle, 0, 0, 0, 0, (uint16_t)width, (uint16_t)height,
        bgfx::copy(finalData.data(), expectedSize)
    );

    loaddata->textureHandle = textureHandle;
    loaddata->format = targetFormat;
    loaddata->data = std::move(finalData);
    loaddata->bitsPerPixel = 32;

    bimg::imageFree(imageContainer);
}

void gbe::gfx::TextureLoader::UnLoadAsset_(TextureData* data)
{
    if (bgfx::isValid(data->textureHandle)) {
        bgfx::destroy(data->textureHandle);
    }
}

void gbe::gfx::TextureLoader::AssignSelfAsLoader()
{
    AssetLoader::AssignSelfAsLoader();
    asset::all_asset_loaders.insert_or_assign(asset::TEXTURE, this);

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
		.dimensions = Vector2Int(width, height)
    };
}

gbe::gfx::TextureData& gbe::gfx::TextureLoader::GetDefaultImage() {
    return static_cast<gbe::gfx::TextureLoader*>(active_instance)->defaultImage;
}