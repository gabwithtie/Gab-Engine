#pragma once

// BGFX: Include the library and its platform-specific initialization header
#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include <vector>

#include "Graphics/Renderer.h"
#include "ScreenUtil.h"

namespace gbe {
	namespace gfx {
		namespace bgfx_gab {

			class ForwardRenderer : public Renderer {

				// BGFX: Define the View IDs for each rendering pass
				enum RenderViewId
				{
					VIEW_MAIN_PASS = 20,
					VIEW_GBUFFER_PASS = 5,
					VIEW_SSAO_PASS = 6,
					VIEW_PP0_PASS = 7,
					VIEW_PP1_PASS = 8,
					VIEW_DEBUG_BLITTER = 0
				};

			private:
				//Settings
				int max_lights = 10;
				int shadow_map_resolution = 1024;
				std::vector<bgfx::ViewId> available_views;

				//============BGFX=======================//
				// BGFX: Render target handle for the main pass (the color buffer for the final scene)
				ShaderData shadow_shader;
				ShaderData ssao_shader;
				ShaderData bilateralblur_shader;
				ShaderData gbuffer_shader;

				//=============RUNTIME===============//
				DrawCall* line_call;
				DrawCall* skybox_call;

				std::vector<Matrix4> light_view_arr;
				std::vector<Matrix4> light_proj_arr;
				std::vector<Vector3> light_color_arr;
				std::vector<int> light_type_arr;
				std::vector<int> light_is_square_arr;
				std::vector<float> light_nearclip_arr;
				std::vector<float> light_range_arr;
				std::vector<float> light_bias_min_arr;
				std::vector<float> light_bias_mult_arr;
				std::vector<float> light_cone_inner_arr;
				std::vector<float> light_cone_outer_arr;

				//============DYNAMIC============//
				// BGFX: Textures used as attachments for the Frame Buffers
				bgfx::FrameBufferHandle m_gbufferFB = BGFX_INVALID_HANDLE;
				TextureData m_gbufferNormal = BGFX_INVALID_HANDLE;
				TextureData m_gbufferDepth = BGFX_INVALID_HANDLE;
				TextureData m_ssaoTexture;
				bgfx::FrameBufferHandle m_ssaoFB;

				TextureData m_pp_0 = BGFX_INVALID_HANDLE;
				TextureData m_pp_1 = BGFX_INVALID_HANDLE;
				bgfx::FrameBufferHandle m_ppFB_0;
				bgfx::FrameBufferHandle m_ppFB_1;
				ShaderData curppshader;
				int pp_switch = 1;
				inline TextureData& InitPP(ShaderData& ppshaderdata) {
					TextureData& selectedpp = m_pp_0;
					RenderViewId selectedid = VIEW_PP0_PASS;

					if (pp_switch == 1) selectedpp = m_pp_1;
					if (pp_switch == 1) selectedid = VIEW_PP1_PASS;
					
					curppshader = ppshaderdata;
					bgfx::setViewClear(selectedid, BGFX_CLEAR_COLOR, 0xffffffff, 1.0f, 0);
					bgfx::setViewTransform(selectedid, nullptr, nullptr);
					
					return selectedpp;
				}
				inline TextureData& GetPP() {
					TextureData& selectedpp = m_pp_0;
					if (pp_switch == 1) selectedpp = m_pp_1;
					return selectedpp;
				}
				inline TextureData& GetPrevPP() {
					TextureData& selectedpp = m_pp_1;
					if (pp_switch == 1) selectedpp = m_pp_0;
					return selectedpp;
				}
				inline void SubmitPP() {
					RenderViewId selectedid = VIEW_PP0_PASS;
					if (pp_switch == 1) selectedid = VIEW_PP1_PASS;
					RenderFullscreenPass(selectedid, curppshader.programHandle);

					pp_switch++;
					pp_switch %= 2;
				}

				bgfx::FrameBufferHandle m_mainPassFBO = BGFX_INVALID_HANDLE;
				TextureData m_mainColorTexture = BGFX_INVALID_HANDLE;
				TextureData m_mainDepthTexture = BGFX_INVALID_HANDLE;

				std::vector<Vector4>  m_ssao_kernel_data;

				bgfx::TextureHandle m_shadowArrayTexture = BGFX_INVALID_HANDLE;
				std::vector<TextureData> m_debugShadowTextures;

				bgfx::UniformHandle m_shadowArraySampler;
				std::vector<bgfx::FrameBufferHandle> m_shadowbuffers;

				// BGFX: Vertex buffer for lines
				bgfx::DynamicVertexBufferHandle m_line_vbh = BGFX_INVALID_HANDLE;

				Vector2Int resolution;
			public:
				ForwardRenderer(const GraphicsRenderInfo& passinfo);

				// Inherited via Renderer
				void RenderFrame(const SceneRenderInfo& frameinfo, GraphicsRenderInfo& passinfo) override;
				TextureData ReloadFrame(Vector2Int reso) override;
				void InitializeAssetRequests() override;

				inline ~ForwardRenderer() {
					// BGFX: Clean up created resources
					if (bgfx::isValid(m_line_vbh)) {
						bgfx::destroy(m_line_vbh);
					}
					
					if (bgfx::isValid(m_mainPassFBO)) {
						bgfx::destroy(m_mainPassFBO);
						bgfx::destroy(m_mainColorTexture.textureHandle);
						bgfx::destroy(m_mainDepthTexture.textureHandle);
					}
				}
			};
		}
	}
}