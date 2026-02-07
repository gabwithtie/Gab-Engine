#include "TexturePainter.h"
#include <limits>

namespace gbe::gfx::bgfx_gab {
    TexturePainter TexturePainter::instance;

    TexturePainter::TexturePainter() {
        // We no longer pre-allocate a massive fixed buffer here.
    }

    void TexturePainter::Draw() {
        if (!instance.renderer || !instance.target_texture) return;

        instance.renderer->SetCpuPassMode(Renderer::CPU_PASS_MODE::PASS_UV);
        const auto& uv_data = instance.renderer->Get_localarea_cpu_data();
        if (uv_data.empty()) return;

        int texW = (int)instance.target_texture->width;
        int texH = (int)instance.target_texture->height;

        // 1. Calculate Bounds with absolute clamping
        int min_x = texW, max_x = 0;
        int min_y = texH, max_y = 0;

        for (const auto& uv_8 : uv_data) {
            auto uv = uv_8.ToVector4();
            // Clamp UVs strictly to 0-1 to prevent floating point overflow
            float u = std::clamp(uv.r, 0.0f, 1.0f);
            float v = std::clamp(uv.g, 0.0f, 1.0f);

            int px = static_cast<int>(u * (texW - 1));
            int py = static_cast<int>(v * (texH - 1));

            min_x = std::min(min_x, px); max_x = std::max(max_x, px);
            min_y = std::min(min_y, py); max_y = std::max(max_y, py);
        }

        // 2. Finalize Dimensions
        uint16_t x = (uint16_t)min_x;
        uint16_t y = (uint16_t)min_y;
        uint16_t width = (uint16_t)(max_x - min_x + 1);
        uint16_t height = (uint16_t)(max_y - min_y + 1);

        // 3. Get Format Info and Allocate
        uint32_t bpp = instance.target_texture->bitsPerPixel / 8;

        // Safety check: if bpp is 0 (unsupported format), abort
        if (bpp == 0) return;

        uint32_t expectedSize = width * height * bpp;
        instance.region_buffer = std::vector<uint8_t>(expectedSize, 0);

        for (uint16_t ly = 0; ly < height; ++ly) 
            for (uint16_t lx = 0; lx < width; ++lx) {

                // The offset in BYTES
                uint32_t pixelOffset = (ly * width + lx) * bpp;

                // CRITICAL SAFETY: Check if the offset is within the vector
                if (pixelOffset + bpp <= instance.region_buffer.size()) {
                    switch (instance.target_texture->format) {
                    case bgfx::TextureFormat::RGBA8: {
                        instance.region_buffer[pixelOffset + 0] = 255; // R
                        instance.region_buffer[pixelOffset + 1] = 0;
                        instance.region_buffer[pixelOffset + 2] = 0;
                        instance.region_buffer[pixelOffset + 3] = 255; // A
                        break;
                    }
                    case bgfx::TextureFormat::RGBA16: {
                        // Use pointer arithmetic on the casted type to avoid manual offset math
                        uint16_t* p16 = reinterpret_cast<uint16_t*>(&instance.region_buffer[pixelOffset]);
                        p16[0] = 65535; // R
                        p16[1] = 0;
                        p16[2] = 0;
                        p16[3] = 65535; // A
                        break;
                    }
                    default: break;
                    }
                }
            }
        

        instance.x = x;
        instance.y = y;
        instance.width = width;
        instance.height = height;
    }

    void TexturePainter::Commit() {
        if (instance.region_buffer.size() == 0)
            return;

        bgfx::updateTexture2D(
            instance.target_texture->textureHandle,
            0, 0,
            instance.x, instance.y,
            instance.width, instance.height,
            bgfx::copy(instance.region_buffer.data(), instance.region_buffer.size())
        );
    }
}