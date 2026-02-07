#include "TexturePainter.h"
#include <limits>

namespace gbe::gfx::bgfx_gab {
    TexturePainter TexturePainter::instance;

    TexturePainter::TexturePainter() {
        // We no longer pre-allocate a massive fixed buffer here.
    }

    void TexturePainter::Draw() {
        const auto& uv_data = instance.renderer->Get_localarea_cpu_data();
        if (uv_data.empty()) return;

        int texW = instance.target_texture->width;
        int texH = instance.target_texture->height;

        // 1. Calculate Bounds with Safety Clamping
        int min_x = texW, max_x = 0;
        int min_y = texH, max_y = 0;

        for (const auto& uv_8 : uv_data) {
            auto uv = uv_8.ToVector4();

            int px = std::clamp(static_cast<int>(uv.r * (texW - 1)), 0, texW - 1);
            int py = std::clamp(static_cast<int>(uv.g * (texH - 1)), 0, texH - 1);
            min_x = std::min(min_x, px); max_x = std::max(max_x, px);
            min_y = std::min(min_y, py); max_y = std::max(max_y, py);
        }

        // 2. Validate Dimensions
        // Ensure we don't have a 0-width or 0-height update region
        if (max_x < min_x || max_y < min_y) return;

        instance.x = (uint16_t)min_x;
        instance.y = (uint16_t)min_y;
        instance.width = (uint16_t)(max_x - min_x + 1);
        instance.height = (uint16_t)(max_y - min_y + 1);

        // 3. Safety Check: Verify Texture Handle
        if (!bgfx::isValid(instance.target_texture->textureHandle)) return;

        // 4. Calculate Expected Size
        // Assuming RGBA8 (4 bytes per pixel). Adjust if your texture format is different.
        uint32_t bytesPerPixel = 4;
        uint32_t expectedSize = instance.width * instance.height * bytesPerPixel;

        instance.region_buffer = std::vector<uint8_t>(expectedSize, 0);

        for (const auto& uv_8 : uv_data) {
            auto uv = uv_8.ToVector4();

            int px = static_cast<int>(uv.r * (instance.target_texture->width - 1));
            int py = static_cast<int>(uv.g * (instance.target_texture->height - 1));

            // Map global pixel coord to local region buffer coord
            int local_x = px - min_x;
            int local_y = py - min_y;

            int i_index = (local_y * instance.width + local_x) * 4;
            auto index = static_cast<std::vector<uint8_t, std::allocator<uint8_t>>::size_type>(i_index);

            // Paint local pixel Red
            instance.region_buffer[index + 0] = 255;
            instance.region_buffer[index + 1] = 0;
            instance.region_buffer[index + 2] = 0;
            instance.region_buffer[index + 3] = 255;
        }
    }

    void TexturePainter::Commit() {
        // In this localized version, Draw() handles the update. 
        // If you need a deferred commit, you would store the region_buffer 
        // and the bounding box coordinates in the instance.

        // 4. Immediate Blit (Commit) for the sub-region
        // We use the x, y offsets and width, height to tell bgfx exactly where to paint
        bgfx::updateTexture2D(
            instance.target_texture->textureHandle,
            0, 0,
            instance.x, instance.y,
            instance.width, instance.height,
            bgfx::copy(instance.region_buffer.data(), instance.region_buffer.size())
        );
    }
}