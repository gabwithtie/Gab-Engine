#include "TexturePainter.h"
#include "TexturePainter.h"
#include <limits>

#include "Graphics/gbe_graphics.h"

namespace gbe::gfx::bgfx_gab {

	TexturePainter_bgfx TexturePainter_bgfx::implemented_instance;

    void TexturePainter_bgfx::Init()
    {
        instance = &implemented_instance;
    }

    TexturePainter_bgfx::TexturePainter_bgfx()
    {
        const auto Draw = [=](Vector2Int pos) {
            if (!RenderPipeline::GetRenderer() || !this->target_texture) return;

            RenderPipeline::GetRenderer()->SetCpuPassMode(Renderer::CPU_PASS_MODE::PASS_UV);
            const auto& uv_data = RenderPipeline::GetRenderer()->Get_localarea_cpu_data();
            if (uv_data.empty()) return;

            int texW = (int)this->target_texture->width;
            int texH = (int)this->target_texture->height;

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
            DrawCommand newcmd;

            newcmd.pixel_screen_position = pos;
            newcmd.x = (uint16_t)min_x;
            newcmd.y = (uint16_t)min_y;
            newcmd.width = (uint16_t)(max_x - min_x + 1);
            newcmd.height = (uint16_t)(max_y - min_y + 1);

            // 3. Get Format Info and Allocate
            uint32_t bpp = this->target_texture->bitsPerPixel / 8;

            // Safety check: if bpp is 0 (unsupported format), abort
            if (bpp == 0) return;

            uint32_t expectedSize = newcmd.width * newcmd.height * bpp;
            newcmd.cpu_buffer = std::vector<uint8_t>(expectedSize, 0);

            for (uint16_t ly = 0; ly < newcmd.height; ++ly)
                for (uint16_t lx = 0; lx < newcmd.width; ++lx) {

                    // The offset in BYTES
                    uint32_t pixelOffset = (ly * newcmd.width + lx) * bpp;

                    // CRITICAL SAFETY: Check if the offset is within the vector
                    if (pixelOffset + bpp <= newcmd.cpu_buffer.size()) {
                        switch (this->target_texture->format) {
                        case bgfx::TextureFormat::RGBA8: {
                            newcmd.cpu_buffer[pixelOffset + 0] = 255; // R
                            newcmd.cpu_buffer[pixelOffset + 1] = 0;
                            newcmd.cpu_buffer[pixelOffset + 2] = 0;
                            newcmd.cpu_buffer[pixelOffset + 3] = 255; // A
                            break;
                        }
                        case bgfx::TextureFormat::RGBA16: {
                            // Use pointer arithmetic on the casted type to avoid manual offset math
                            uint16_t* p16 = reinterpret_cast<uint16_t*>(&newcmd.cpu_buffer[pixelOffset]);
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

            this->draw_queue.push_back(newcmd);

            };

		this->draw_callback = Draw;

        const auto Commit = [=](DrawCommand cmd) {
            bgfx::updateTexture2D(
                this->target_texture->textureHandle,
                0, 0,
                cmd.x, cmd.y,
                cmd.width, cmd.height,
                bgfx::copy(cmd.cpu_buffer.data(), cmd.cpu_buffer.size())
            );
            };

		this->commit_callback = Commit;
    }
}