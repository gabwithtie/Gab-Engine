#include "TextureBlend.h"

#include "../ScreenUtil.h"
#include "../forwardrenderer.h"

#include "Graphics/gbe_graphics.h"

gbe::gfx::bgfx_gab::TextureBlend_bgfx::TextureBlend_bgfx()
{
	this->instance = this;

	this->impl_blend = [=](TextureData* s_texA, TextureData* s_texB, TextureData* _dst, BlendMode mode) {
		ShaderData* blendshader = ShaderLoader::GetAssetRuntimeData("bufferblend");

		auto newtexdata = TextureData{
			.textureHandle = bgfx::createTexture2D(_dst->dimensions.x, _dst->dimensions.y, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT)
		};
		auto newfb = bgfx::createFrameBuffer(1, &newtexdata.textureHandle, false);
		bgfx::setViewFrameBuffer(ForwardRenderer::VIEW_BLENDER, newfb);
		bgfx::setViewRect(ForwardRenderer::VIEW_BLENDER, 0, 0, _dst->dimensions.x, _dst->dimensions.y);

		bgfx::setViewClear(ForwardRenderer::VIEW_BLENDER, BGFX_CLEAR_COLOR, 0x00000000, 1.0f, 0);
		bgfx::setViewTransform(ForwardRenderer::VIEW_BLENDER, nullptr, nullptr);

		blendshader->ApplyTextureOverride(s_texA, "tex_buffer_a", 0);
		blendshader->ApplyTextureOverride(s_texB, "tex_buffer_b", 1);
		Vector4 blend_params(0, 0, 0, 0);
		blendshader->ApplyOverride(blend_params, "u_blend_params");
		
		RenderFullscreenPass(ForwardRenderer::VIEW_BLENDER, blendshader->programHandle);

		bgfx::destroy(newfb);
		};
}
