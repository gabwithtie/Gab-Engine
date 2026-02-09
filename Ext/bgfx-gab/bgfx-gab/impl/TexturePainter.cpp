#include "TexturePainter.h"
#include <cmath> // for sqrt

#include "Graphics/RenderPipeline.h"

namespace gbe::gfx::bgfx_gab {

    TexturePainter_bgfx::TexturePainter_bgfx() {
        this->instance = this;

        // Triggered when the user selects a new texture to paint on
        this->impl_change_tex = [=]() {
            if (instance->GetTargetTexture() == nullptr) return;

            auto assetId = instance->GetTargetTexture()->Get_assetId();
            auto tex_data = TextureLoader::GetAssetRuntimeData(assetId);

            // Store info and resize local buffer
            m_info.width = tex_data->dimensions.x;
            m_info.height = tex_data->dimensions.y;
            m_info.format = tex_data->format;

            uint32_t bpp = tex_data->bitsPerPixel / 8;
            m_localBuffer = tex_data->data;
            };

        this->impl_draw = [=](Vector2Int pos) {
            if (!RenderPipeline::GetRenderer() || !this->target_texture) return;

            RenderPipeline::GetRenderer()->SubmitCpuDataRequest({
                .override_id = "",
                .cpu_pass_mode = Renderer::CPU_PASS_MODE::PASS_UV,
                .cursor_pixel_pos = pos,
                .rect_size = brush_size,
                .callback = [&](Renderer::CpuDataResponse& response) {
                    const auto& uv_data = response.cpu_data;
                    if (uv_data.empty()) return;

                    auto tex_data = TextureLoader::GetAssetRuntimeData(this->target_texture->Get_assetId());
                    int texW = (int)m_info.width;
                    int texH = (int)m_info.height;
                    uint32_t bpp = tex_data->bitsPerPixel / 8;

                    // 1. Determine Brush Center (Average UV hits)
                    float avgU = 0, avgV = 0;
                    for (auto& uv_8 : uv_data) {
                        auto uv = uv_8.ToVector4();
                        avgU += uv.r; avgV += uv.g;
                    }
                    avgU /= uv_data.size();
                    avgV /= uv_data.size();

                    float centerPx = avgU * (texW - 1);
                    float centerPy = avgV * (texH - 1);
                    float radius = (float)this->brush_size / 2.0f;

                    // 2. Define dirty rect for bgfx::update (the area we actually modified)
                    int min_x = std::max(0, (int)(centerPx - radius));
                    int max_x = std::min(texW - 1, (int)(centerPx + radius));
                    int min_y = std::max(0, (int)(centerPy - radius));
                    int max_y = std::min(texH - 1, (int)(centerPy + radius));

                    uint16_t updateW = (uint16_t)(max_x - min_x + 1);
                    uint16_t updateH = (uint16_t)(max_y - min_y + 1);

                    // 3. Prepare temporary buffer for the sub-region update
                    std::vector<uint8_t> updateBuffer(updateW * updateH * bpp);

					bool modified = false;

                    for (int y = min_y; y <= max_y; ++y) {
                        for (int x = min_x; x <= max_x; ++x) {
                            // Calculate Distance for Smoothness
                            float dx = (float)x - centerPx;
                            float dy = (float)y - centerPy;
                            float dist = std::sqrt(dx * dx + dy * dy);

                            // Hardness/Falloff (0.0 to 1.0)
                            float alpha = std::clamp(1.0f - (dist / radius), 0.0f, 1.0f);

                            // Apply Brush Strength
                            alpha *= this->brush_strength;

                            // Indices
                            uint32_t globalIdx = (y * texW + x) * bpp;
                            uint32_t localIdx = ((y - min_y) * updateW + (x - min_x)) * bpp;
                            Vector4 brushCol = TexturePainter::GetBrushColor();

                            // Inside the loop in TexturePainter_bgfx::TexturePainter_bgfx()
                            if (tex_data->format == bgfx::TextureFormat::RGBA8) {

                                for (int c = 0; c < 3; ++c) { // RGB
                                    float oldCol = (float)m_localBuffer[globalIdx + c];
                                    float targetCol = brushCol[c] * 255.0f;

                                    // Apply smooth lerp using the calculated alpha falloff
                                    m_localBuffer[globalIdx + c] = (uint8_t)(oldCol + (targetCol - oldCol) * alpha);
                                }

                                // For the Alpha channel of the texture, you might want to blend or keep it at 255
                                m_localBuffer[globalIdx + 3] = 255;

                                memcpy(&updateBuffer[localIdx], &m_localBuffer[globalIdx], bpp);
								modified = true;
                            }
                            if (tex_data->format == bgfx::TextureFormat::RGBA16) {
                                uint16_t* localPixel = reinterpret_cast<uint16_t*>(&m_localBuffer[globalIdx]);
                                uint16_t* updatePixel = reinterpret_cast<uint16_t*>(&updateBuffer[localIdx]);

                                for (int c = 0; c < 3; ++c) { // RGB channels
                                    // Convert the 0.0-1.0 float brush color to 0-65535
                                    float targetCol = brushCol[c] * 65535.0f;
                                    float oldCol = static_cast<float>(localPixel[c]);

                                    // Linear interpolation: Old + (Target - Old) * Alpha
                                    // We do the math in float to maintain precision and avoid 16-bit overflow
                                    localPixel[c] = static_cast<uint16_t>(oldCol + (targetCol - oldCol) * alpha);
                                }

                                // Set or blend alpha channel (index 3)
                                // Often for 16-bit textures, you want to keep the alpha at full (65535)
                                localPixel[3] = 65535;

                                // 2. Copy the resulting 8 bytes (4 channels * 2 bytes) to the update buffer
                                std::memcpy(updatePixel, localPixel, 8);
                                modified = true;
                            }
                        }
                    }

                    if (modified)
                        // 4. Push only the modified sub-region to GPU
                        bgfx::updateTexture2D(
                            tex_data->textureHandle, 0, 0,
                            (uint16_t)min_x, (uint16_t)min_y,
                            updateW, updateH,
                            bgfx::copy(updateBuffer.data(), updateBuffer.size())
                        );
                }
                });
            };
    }
}