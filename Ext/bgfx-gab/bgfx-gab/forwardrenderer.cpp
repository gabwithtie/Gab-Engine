#include "forwardrenderer.h"

#include "Graphics/gbe_graphics.h"

#include <random> // Added for SSAO kernel generation

#include "ScreenUtil.h"

gbe::gfx::bgfx_gab::ForwardRenderer::ForwardRenderer(const GraphicsRenderInfo& passinfo) {
	m_line_vbh = bgfx::createDynamicVertexBuffer(passinfo.max_lines, s_VERTEXLAYOUT, BGFX_BUFFER_NONE);

	// 1. Create a single Texture Array (10 layers)
	// The 'max_lights' parameter here defines the number of layers
	m_shadowArrayTexture = bgfx::createTexture2D(
		shadow_map_resolution,
		shadow_map_resolution,
		false,
		max_lights,
		bgfx::TextureFormat::D16,
		BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
	);

	// 2. Create one single sampler uniform for the array
	m_shadowArraySampler = bgfx::createUniform("light_map", bgfx::UniformType::Sampler);

	// 3. Create a FrameBuffer for EACH layer
	m_shadowbuffers.resize(max_lights);
	for (int i = 0; i < max_lights; ++i) {
		bgfx::Attachment at;
		// init(handle, access, layer, mip, resolve)
		at.init(m_shadowArrayTexture, bgfx::Access::Write, (uint16_t)i);

		// Use the Attachment overload of createFrameBuffer
		m_shadowbuffers[i] = bgfx::createFrameBuffer(1, &at, false);
	}

	for (size_t i = 10; i < 20; i++)
	{
		available_views.push_back(i);
	}

	m_debugShadowTextures.resize(max_lights);
	for (int i = 0; i < max_lights; ++i) {
		m_debugShadowTextures[i] = bgfx::createTexture2D(
			shadow_map_resolution,
			shadow_map_resolution,
			false, 1,
			bgfx::TextureFormat::D16,
			BGFX_TEXTURE_BLIT_DST // Required for blitting
		);
	}

	light_view_arr.resize(max_lights);
	light_proj_arr.resize(max_lights);
	light_color_arr.resize(max_lights);
	light_type_arr.resize(max_lights);
	light_is_square_arr.resize(max_lights);
	light_nearclip_arr.resize(max_lights);
	light_range_arr.resize(max_lights);
	light_bias_min_arr.resize(max_lights);
	light_bias_mult_arr.resize(max_lights);
	light_cone_inner_arr.resize(max_lights);
	light_cone_outer_arr.resize(max_lights);

	// Generate 64 random samples in a hemisphere
	m_ssao_kernel_data.resize(64);
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	for (unsigned int i = 0; i < 64; ++i) {
		Vector3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
		sample.Normalize();
		sample *= randomFloats(generator);
		float scale = (float)i / 64.0f;
		scale = 0.1f + scale * scale * (1.0f - 0.1f); // Lerp for distribution
		m_ssao_kernel_data[i] = Vector4(sample.x * scale, sample.y * scale, sample.z * scale, 0.0f);
	}
}

void gbe::gfx::bgfx_gab::ForwardRenderer::InitializeAssetRequests()
{
	//Register Line Drawcall
	this->line_call = RenderPipeline::RegisterDrawCall(nullptr, asset::Material::GetAssetById("line"), 0);
	RenderPipeline::PrepareCall(this->line_call);

	this->skybox_call = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("cube"), asset::Material::GetAssetById("skybox_gradient"), 0);
	RenderPipeline::PrepareCall(this->skybox_call);

	shadow_shader = ShaderLoader::GetAssetRuntimeData("shadow");
	ssao_shader = ShaderLoader::GetAssetRuntimeData("ssao");
	gbuffer_shader = ShaderLoader::GetAssetRuntimeData("gbuffer");
}

void gbe::gfx::bgfx_gab::ForwardRenderer::RenderFrame(const SceneRenderInfo& frameinfo, GraphicsRenderInfo& passinfo)
{
	//helper function for drawbatch
	const auto drawbatch = [&passinfo](std::pair<DrawCall*, std::vector<void*>> shaderset) {
		const auto& drawcall = shaderset.first;

		// 3. Bind Mesh
		const auto& curmesh = MeshLoader::GetAssetRuntimeData(drawcall->get_mesh()->Get_assetId());
		bgfx::setIndexBuffer(curmesh.index_vbh);
		bgfx::setVertexBuffer(0, curmesh.vertex_vbh);

		// 1. Get the number of instances
		uint32_t instanceCount = (uint32_t)shaderset.second.size();
		if (instanceCount == 0) return false;

		// 2. Allocate an Instance Data Buffer
		// We need 64 bytes (16 floats) per instance for a Matrix4
		bgfx::InstanceDataBuffer idb;
		uint32_t stride = 64;
		bgfx::allocInstanceDataBuffer(&idb, instanceCount, stride);

		// 3. Fill the buffer with your matrices
		uint8_t* data = idb.data;
		for (const auto& call_ptr : shaderset.second)
		{
			Matrix4 modelMatrix = passinfo.matrix_map[call_ptr];
			memcpy(data, &modelMatrix, stride);
			data += stride;
		}

		// 4. Set geometry and the instance buffer
		bgfx::setIndexBuffer(curmesh.index_vbh);
		bgfx::setVertexBuffer(0, curmesh.vertex_vbh);
		bgfx::setInstanceDataBuffer(&idb); // This replaces setTransform!

		return true;
		};

	// BGFX: Frame submission must start with a `bgfx::touch` or `bgfx::frame`
	bgfx::touch(VIEW_MAIN_PASS); // Touch one view to start frame

	//================== 1. G-BUFFER PRE-PASS (For SSAO) ========================//
	bgfx::setViewClear(VIEW_GBUFFER_PASS, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
	bgfx::setViewTransform(VIEW_GBUFFER_PASS, (const float*)&frameinfo.viewmat, (const float*)&frameinfo.projmat);

	for (const auto& shaderset : passinfo.sortedcalls[0]) {
		drawbatch(shaderset);
		bgfx::setState(BGFX_STATE_DEFAULT);
		bgfx::submit(VIEW_GBUFFER_PASS, gbuffer_shader.programHandle);
	}

	//================== 2. SSAO CALCULATION PASS ========================//
	bgfx::setViewTransform(VIEW_SSAO_PASS, nullptr, nullptr); // Screen space
	ssao_shader.ApplyTextureOverride(TextureData{
		.textureHandle = m_gbufferNormal
		},
		"tex_normal",
		0);
	ssao_shader.ApplyTextureOverride(TextureData{
		.textureHandle = m_gbufferDepth
		},
		"tex_depth",
		1);

	ssao_shader.ApplyOverrideArray(m_ssao_kernel_data.data(), "u_kernel", 64);

	// Params: x=radius, y=bias, z=screenWidth, w=screenHeight
	Vector4 params(0.5f, 0.025f, (float)resolution.x, (float)resolution.y);
	ssao_shader.ApplyOverride(params, "u_ssao_params");

	RenderFullscreenPass(VIEW_SSAO_PASS, ssao_shader.programHandle);

	//==================SHADOW PASS [VIEW_SHADOW_PASS]========================//
	for (size_t i = 0; i < max_lights; i++)
	{
		if (i >= available_views.size())
			break;

		bgfx::ViewId shadowViewId = available_views[i];

		if (i == frameinfo.lightdatas.size())
			break;

		bgfx::setViewFrameBuffer(shadowViewId, m_shadowbuffers[i]);
		bgfx::setViewRect(shadowViewId, 0, 0, shadow_map_resolution, shadow_map_resolution);
		bgfx::setViewClear(shadowViewId, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);

		const auto& light = frameinfo.lightdatas[i];

		light->UpdateContext(frameinfo.viewmat, frameinfo.projmat_lightusage);
		Matrix4 lightViewMat = light->GetViewMatrix();
		Matrix4 lightProjMat = light->GetProjectionMatrix();
		bgfx::setViewTransform(shadowViewId, (const float*)&lightViewMat, (const float*)&lightProjMat);
		
		Matrix4 lightProjView = lightProjMat * lightViewMat;

		for (const auto& shaderset : passinfo.sortedcalls[0])
		{
			drawbatch(shaderset);

			// BGFX: Set State (Depth Test, Culling, etc.)
			bgfx::setState(
				BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CW
			);

			bgfx::submit(shadowViewId, this->shadow_shader.programHandle);
		}
	}

	//==================MAIN PASS [VIEW_MAIN_PASS]========================//
	// Clear the main pass color/depth before rendering
	bgfx::setViewClear(VIEW_MAIN_PASS, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x181818ff, 1.0f, 0);
	bgfx::setViewTransform(VIEW_MAIN_PASS, (const float*)&frameinfo.viewmat, (const float*)&frameinfo.projmat);

	//===============LINE PASS [VIEW_LINE_PASS]
	if (passinfo.lines_this_frame.size() > 0 && false) {

		// BGFX: Update dynamic vertex buffer
		bgfx::update(m_line_vbh, 0, bgfx::makeRef(passinfo.lines_this_frame.data(), (uint32_t)(passinfo.lines_this_frame.size() * sizeof(asset::data::Vertex))));

		auto lineshaderasset = this->line_call->get_material()->Get_load_data().shader;
		const auto& lineshader = ShaderLoader::GetAssetRuntimeData(lineshaderasset->Get_assetId());

		// 2. Set Vertex Buffer
		bgfx::setVertexBuffer(0, m_line_vbh, 0, (uint32_t)passinfo.lines_this_frame.size());

		// 3. Set Transform (Identity for lines)
		bgfx::setTransform(nullptr);

		// 4. Set State (for lines)
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_PT_LINES // Primitive type lines
		);

		// 5. Submit
		bgfx::submit(VIEW_MAIN_PASS, lineshader.programHandle);

		passinfo.lines_this_frame.clear();
	}
	//=================END OF LINE PASS

	//=================SKYBOX PASS [VIEW_SKYBOX_PASS]
	if (false)
	{
		const auto& skyboxmesh = MeshLoader::GetAssetRuntimeData(this->skybox_call->get_mesh()->Get_assetId());
		auto skyboxshaderasset = this->skybox_call->get_material()->Get_load_data().shader;
		const auto& skyboxshader = ShaderLoader::GetAssetRuntimeData(skyboxshaderasset->Get_assetId());

		// 2. Bind Mesh
		bgfx::setIndexBuffer(skyboxmesh.index_vbh);
		bgfx::setVertexBuffer(0, skyboxmesh.vertex_vbh);

		// 3. Set Transform (Identity)
		bgfx::setTransform(nullptr);

		// 4. Set State (disable depth write, only depth test to pass if Z is 1.0)
		bgfx::setState(0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_DEPTH_TEST_LEQUAL // Less or Equal to draw skybox at max depth
			| BGFX_STATE_CULL_CW
		);

		// 5. Submit
		bgfx::submit(VIEW_MAIN_PASS, skyboxshader.programHandle);
	}
	//=================END OF SKYBOX PASS

	for (const auto& shaderset : passinfo.sortedcalls[0])
	{
		const auto& drawcall = shaderset.first;
		const auto& currentshaderdata = drawcall->get_shaderdata();

		drawcall->SyncMaterialData();

		// Set light data uniforms
		
		for (size_t i = 0; i < max_lights; i++)
		{
			// The original code was updating light uniforms for max_lights, even if not present,
			// which is necessary for uniform arrays in the shader.
			if (i >= frameinfo.lightdatas.size())
			{
				light_color_arr[i] = Vector3(0, 0, 0);
				continue;
			}

			const auto& light = frameinfo.lightdatas[i];

			light_color_arr[i] = light->color;
			light_view_arr[i] = light->GetViewMatrix();
			light_proj_arr[i] = light->GetProjectionMatrix();
			light_type_arr[i] = light->type;
			light_is_square_arr[i] = light->square_project;
			light_nearclip_arr[i] = light->near_clip;
			light_range_arr[i] = light->range;
			light_bias_min_arr[i] = light->bias_min;
			light_bias_mult_arr[i] = light->bias_mult;
			light_cone_inner_arr[i] = light->angle_inner;
			light_cone_outer_arr[i] = light->angle_outer;
		}

		bgfx::setTexture(0, m_shadowArraySampler, m_shadowArrayTexture);

		drawcall->ApplyOverrideArray<Matrix4>(light_view_arr.data(), "light_view", max_lights);
		drawcall->ApplyOverrideArray<Matrix4>(light_proj_arr.data(), "light_proj", max_lights);
		drawcall->ApplyOverrideArray<Vector3>(light_color_arr.data(), "light_color", max_lights);
		drawcall->ApplyOverrideArray<int>(light_type_arr.data(), "light_type", max_lights);
		drawcall->ApplyOverrideArray<int>(light_is_square_arr.data(), "light_is_square", max_lights);
		drawcall->ApplyOverrideArray<float>(light_nearclip_arr.data(), "light_nearclip", max_lights);
		drawcall->ApplyOverrideArray<float>(light_range_arr.data(), "light_range", max_lights);
		drawcall->ApplyOverrideArray<float>(light_bias_min_arr.data(), "light_bias_min", max_lights);
		drawcall->ApplyOverrideArray<float>(light_bias_mult_arr.data(), "light_bias_mult", max_lights);
		drawcall->ApplyOverrideArray<float>(light_cone_inner_arr.data(), "light_cone_inner", max_lights);
		drawcall->ApplyOverrideArray<float>(light_cone_outer_arr.data(), "light_cone_outer", max_lights);

		drawbatch(shaderset);

		// 5. Submit ONCE for all instances
		drawcall->SyncMaterialData();
		bgfx::setState(BGFX_STATE_DEFAULT);
		bgfx::submit(VIEW_MAIN_PASS, currentshaderdata->programHandle);
	}

	//DEBUG
	for (uint16_t i = 0; i < (uint16_t)max_lights; ++i) {
		// blit(viewId, dst, dstMip, dstX, dstY, dstZ, src, srcMip, srcX, srcY, srcZ, width, height, depth)
		bgfx::blit(
			VIEW_DEBUG_BLITTER,           // The view that performs the copy
			m_debugShadowTextures[i], // Destination: Singular 2D texture
			0, 0, 0, 0,               // Mip 0, X 0, Y 0, Z 0
			m_shadowArrayTexture,     // Source: Your Texture Array
			0, 0, 0, i,               // Mip 0, X 0, Y 0, Layer 'i'
			shadow_map_resolution,
			shadow_map_resolution
		);
	}
}

gbe::gfx::TextureData gbe::gfx::bgfx_gab::ForwardRenderer::ReloadFrame(Vector2Int reso)
{
	// Destroy existing FBOs and textures if any
	if (bgfx::isValid(m_mainPassFBO)) {
		bgfx::destroy(m_mainPassFBO);
		bgfx::destroy(m_mainColorTexture);
		bgfx::destroy(m_mainDepthTexture);
	}

	uint16_t w = (uint16_t)reso.x;
	uint16_t h = (uint16_t)reso.y;

	// Create Main Pass FBO (Color + Depth)
	// Use BGFX_TEXTURE_RT to mark as a render target
	m_mainColorTexture = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT);
	m_mainDepthTexture = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT_WRITE_ONLY);
	bgfx::TextureHandle mainAttachments[] = { m_mainColorTexture, m_mainDepthTexture };
	m_mainPassFBO = bgfx::createFrameBuffer(BX_COUNTOF(mainAttachments), mainAttachments, false);
	bgfx::setViewRect(VIEW_MAIN_PASS, 0, 0, w, h);
	bgfx::setViewFrameBuffer(VIEW_MAIN_PASS, m_mainPassFBO);

	//SSBO
	m_gbufferNormal = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA16F, BGFX_TEXTURE_RT);
	m_gbufferDepth = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24, BGFX_TEXTURE_RT);

	bgfx::TextureHandle gbufferAttachments[] = { m_gbufferNormal, m_gbufferDepth };
	m_gbufferFB = bgfx::createFrameBuffer(BX_COUNTOF(gbufferAttachments), gbufferAttachments, true);

	resolution = reso;

	//DEBUG
	int counter = 0;
	for (const auto& shadowmap : this->m_debugShadowTextures)
	{
		TextureData shadowmap_tex = {
			.textureHandle = shadowmap,
		};
		TextureLoader::RegisterExternal("shadowmap_" + std::to_string(counter), shadowmap_tex);
		counter++;
	}

	// Main Pass Color Texture
	TextureData mainpass_tex = {
		.textureHandle = m_mainColorTexture, // Pass the bgfx handle
	};
	return mainpass_tex;
}
