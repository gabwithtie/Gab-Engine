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
					VIEW_GBUFFER_PASS,
					VIEW_SSAO_PASS,
					VIEW_BLUR0_PASS,
					VIEW_BLUR1_PASS,
					VIEW_MAIN_PASS = 100,
					VIEW_DEBUG_BLITTER
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
				std::vector<Vector4> light_color_arr;
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

				ShaderData curppshader;
				RenderViewId curppview;

				std::vector<bgfx::FrameBufferHandle> m_ppFBs;
				std::unordered_map<RenderViewId, TextureData> m_pp_textures;

				inline void InitPP(ShaderData& ppshaderdata, RenderViewId viewid) {
					curppview = viewid;
					curppshader = ppshaderdata;
					bgfx::setViewClear(viewid, BGFX_CLEAR_COLOR, 0xffffffff, 1.0f, 0);
					bgfx::setViewTransform(viewid, nullptr, nullptr);
				}
				inline void SubmitPP() {
					RenderFullscreenPass(curppview, curppshader.programHandle);
				}
				inline void RegisterPPFramebuffer(RenderViewId viewid, bgfx::TextureFormat::Enum tformat = bgfx::TextureFormat::R8) {
					auto newtexdata = TextureData{
						.textureHandle = bgfx::createTexture2D(resolution.x, resolution.y, false, 1, tformat, BGFX_TEXTURE_RT)
					};
					m_pp_textures.insert_or_assign(viewid, newtexdata);
					auto newfb = bgfx::createFrameBuffer(1, &newtexdata.textureHandle, false);
					m_ppFBs.push_back(newfb);
					bgfx::setViewFrameBuffer(viewid, newfb);
					bgfx::setViewRect(viewid, 0, 0, resolution.x, resolution.y);
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