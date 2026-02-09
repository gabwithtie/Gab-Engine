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
    // Load with Count to get the native format first
    bimg::ImageContainer* imageContainer = imageLoad(pathstr.string().c_str(), bgfx::TextureFormat::Count);

    if (imageContainer == nullptr) {
        throw std::runtime_error("Failed to decode texture: " + pathstr.string());
    }

    // Prepare metadata
    uint32_t width = imageContainer->m_width;
    uint32_t height = imageContainer->m_height;
    loaddata->dimensions = Vector2Int(width, height);

    bgfx::TextureFormat::Enum targetFormat = (bgfx::TextureFormat::Enum)imageContainer->m_format;
    std::vector<uint8_t> finalData;

    // Handle RGBA16 specifically
    if (targetFormat == bgfx::TextureFormat::RGBA16) {
        targetFormat = bgfx::TextureFormat::RGBA8; // Force downsample target

        // Size = Width * Height * 4 channels * 1 byte per channel
        uint32_t targetSize = width * height * 4;
        finalData.resize(targetSize);

        const uint16_t* src16 = reinterpret_cast<const uint16_t*>(imageContainer->m_data);
        uint8_t* dst8 = finalData.data();

        // Mapping 16-bit to 8-bit
        for (size_t i = 0; i < (size_t)width * height * 4; ++i) {
            // Shifting 8 bits right effectively takes the most significant byte
            dst8[i] = static_cast<uint8_t>(src16[i] >> 8);
        }
    }
    else {
        // For other formats, use your signature of imageGetSize
        // We pass a dummy TextureInfo if the function requires it for internal math
        bimg::TextureInfo dummyInfo;
        uint32_t size = bimg::imageGetSize(
            &dummyInfo,
            (uint16_t)width,
            (uint16_t)height,
            1,      // depth
            false,  // cubeMap
            false,  // hasMips
            1,      // numLayers
            (bimg::TextureFormat::Enum)targetFormat
        );

        finalData.assign((uint8_t*)imageContainer->m_data, (uint8_t*)imageContainer->m_data + size);
    }
    // --- DOWNSAMPLE LOGIC END ---

    uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_TEXTURE_BLIT_DST;

    // Create handle using the TARGET format (RGBA8)
    bgfx::TextureHandle textureHandle = bgfx::createTexture2D(
        (uint16_t)width, (uint16_t)height, false, 1,
        targetFormat, flags, nullptr
    );

    if (!bgfx::isValid(textureHandle)) {
        bimg::imageFree(imageContainer);
        throw std::runtime_error("bgfx failed to create texture.");
    }

    // Upload the processed data
    bgfx::updateTexture2D(
        textureHandle, 0, 0, 0, 0, (uint16_t)width, (uint16_t)height,
        bgfx::copy(finalData.data(), finalData.size())
    );

    loaddata->textureHandle = textureHandle;
    loaddata->format = targetFormat;
    loaddata->data = std::move(finalData); // Store the processed 8-bit data
    loaddata->bitsPerPixel = 32; // RGBA8 is always 32

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