#include "forwardrenderer.h"

#include "Graphics/gbe_graphics.h"

#include <random> // Added for SSAO kernel generation

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

	for (size_t i = VIEW_SHADOW_PASS; i < 20; i++)
	{
		available_views.push_back(i);
	}

	m_debugShadowTextures.resize(max_lights);
	for (int i = 0; i < max_lights; ++i) {
		m_debugShadowTextures[i] = TextureData{
			.textureHandle = bgfx::createTexture2D(
				shadow_map_resolution,
				shadow_map_resolution,
				false, 1,
				bgfx::TextureFormat::D16,
				BGFX_TEXTURE_BLIT_DST // Required for blitting
			)
		};

		TextureLoader::RegisterExternal("shadowmap_" + std::to_string(i), m_debugShadowTextures[i]);
	}

	light_view_arr.resize(max_lights);
	light_proj_arr.resize(max_lights);
	light_color_arr.resize(max_lights);
	light_pos_arr.resize(max_lights);
	light_type_arr.resize(max_lights);
	light_is_square_arr.resize(max_lights);
	light_nearclip_arr.resize(max_lights);
	light_range_arr.resize(max_lights);
	light_bias_min_arr.resize(max_lights);
	light_bias_mult_arr.resize(max_lights);
	light_cone_inner_arr.resize(max_lights);
	light_cone_outer_arr.resize(max_lights);

	localarea_id_data.resize(id_texture_size * id_texture_size);

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
	this->line_call = RenderPipeline::RegisterDrawCall(nullptr, asset::Material::GetAssetById("line"));

	this->skybox_call = RenderPipeline::RegisterDrawCall(asset::Mesh::GetAssetById("cube"), asset::Material::GetAssetById("skybox_gradient"));

	shadow_shader = ShaderLoader::GetAssetRuntimeData("shadow"); 
	ssao_shader = ShaderLoader::GetAssetRuntimeData("ssao");
	outline_shader = ShaderLoader::GetAssetRuntimeData("outline");
	gbuffer_shader = ShaderLoader::GetAssetRuntimeData("gbuffer");
	bilateralblur_shader = ShaderLoader::GetAssetRuntimeData("bilateralblur");
	bufferblend_shader = ShaderLoader::GetAssetRuntimeData("bufferblend");
	id_shader = ShaderLoader::GetAssetRuntimeData("id");
}

void gbe::gfx::bgfx_gab::ForwardRenderer::RenderFrame(const SceneRenderInfo& frameinfo, GraphicsRenderInfo& passinfo)
{
	//helper function for drawbatch
	const auto drawbatch = [&passinfo](DrawCall* drawcall, int rendergroup) {
		std::vector<uint32_t> instances;

		for (const auto& instanceid : passinfo.callgroups[drawcall])
		{
			const auto& info = passinfo.infomap[instanceid];

			if(!info.enabled)
				continue;

			auto rendergroup_it = info.rendergroups.find(rendergroup);

			if (rendergroup_it != info.rendergroups.end())
				if (rendergroup_it->second)
					instances.push_back(instanceid);
		}
		
		// 1. Get the number of instances
		uint32_t instanceCount = (uint32_t)instances.size();
		if (instanceCount == 0) return false;

		// 3. Bind Mesh
		const auto& curmesh = MeshLoader::GetAssetRuntimeData(drawcall->get_mesh()->Get_assetId());
		bgfx::setIndexBuffer(curmesh.index_vbh);
		bgfx::setVertexBuffer(0, curmesh.vertex_vbh);

		// 2. Allocate an Instance Data Buffer
		// We need 64 bytes (16 floats) per instance for a Matrix4
		bgfx::InstanceDataBuffer idb;
		uint32_t stride = 64;
		bgfx::allocInstanceDataBuffer(&idb, instanceCount, stride);

		// 3. Fill the buffer with your matrices
		uint8_t* data = idb.data;
		for (const auto& call_ptr : instances)
		{
			Matrix4 modelMatrix = passinfo.infomap[call_ptr].transform;
			memcpy(data, &modelMatrix, stride);
			data += stride;
		}

		// 4. Set geometry and the instance buffer
		bgfx::setIndexBuffer(curmesh.index_vbh);
		bgfx::setVertexBuffer(0, curmesh.vertex_vbh);
		bgfx::setInstanceDataBuffer(&idb); // This replaces setTransform!

		return true;
		};

	const auto drawbuffer = [&passinfo, &frameinfo, drawbatch, this](RenderViewId viewid, int rendergroup, ShaderData _shader) {
		bgfx::setViewClear(viewid, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
		bgfx::setViewTransform(viewid, (const float*)&frameinfo.viewmat, (const float*)&frameinfo.projmat);

		bool submitted = false;

		for (const auto& shaderset : passinfo.callgroups) {
			if (shaderset.second.size() == 0)
				continue;

			if (drawbatch(shaderset.first, rendergroup)) {
				bgfx::setState(BGFX_STATE_DEFAULT);
				bgfx::submit(viewid, _shader.programHandle);
				submitted = true;
			}
		}

		if (!submitted) {
			bgfx::submit(viewid, BGFX_INVALID_HANDLE); // Submit empty to clear
		}
		};

	const auto drawgbuffer = [&drawbuffer, drawbatch, this](GBuffer& buffer, int rendergroup) {
		drawbuffer(buffer.viewId, rendergroup, gbuffer_shader);
		};

	// BGFX: Frame submission must start with a `bgfx::touch` or `bgfx::frame`
	bgfx::touch(VIEW_SCREEN_PASS); // Touch one view to start frame

	//================== 1. G-BUFFER PRE-PASS (For SSAO) ========================//
	drawgbuffer(gbuffer_all, 0);
	drawgbuffer(gbuffer_selected, 1); // outline pass for gbuffer
	//================== ID PASS ========================//
	bgfx::setViewClear(VIEW_ID_PASS, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
	bgfx::setViewTransform(VIEW_ID_PASS, (const float*)&frameinfo.viewmat, (const float*)&frameinfo.projmat);
	for (const auto& shaderset : passinfo.callgroups) {
		auto drawcall = shaderset.first;
		std::vector<uint32_t> instances;

		for (const auto& instanceid : passinfo.callgroups[drawcall])
		{
			const auto& info = passinfo.infomap[instanceid];

			if (!info.enabled)
				continue;

			const auto check_group = [=](int group) {
				auto rendergroup_it = info.rendergroups.find(group);
				if (rendergroup_it != info.rendergroups.end())
					return rendergroup_it->second;

				return false;
				};

			if (check_group(0) || check_group(1))
				instances.push_back(instanceid);
		}

		// 1. Get the number of instances
		uint32_t instanceCount = (uint32_t)instances.size();
		if (instanceCount == 0) continue;

		// 3. Bind Mesh
		const auto& curmesh = MeshLoader::GetAssetRuntimeData(drawcall->get_mesh()->Get_assetId());
		for (const auto& call_ptr : instances)
		{
			bgfx::setIndexBuffer(curmesh.index_vbh);
			bgfx::setVertexBuffer(0, curmesh.vertex_vbh);

			// 2. Allocate an Instance Data Buffer
			// We need 64 bytes (16 floats) per instance for a Matrix4
			bgfx::InstanceDataBuffer idb;
			uint32_t stride = 64;
			bgfx::allocInstanceDataBuffer(&idb, 1, stride);

			// 3. Fill the buffer with your matrices
			uint8_t* data = idb.data;

			Matrix4 modelMatrix = passinfo.infomap[call_ptr].transform;
			memcpy(data, &modelMatrix, stride);
			data += stride;

			// 4. Set geometry and the instance buffer
			bgfx::setIndexBuffer(curmesh.index_vbh);
			bgfx::setVertexBuffer(0, curmesh.vertex_vbh);
			bgfx::setInstanceDataBuffer(&idb); // This replaces setTransform!

			id_shader.ApplyOverride(BRGA_t(call_ptr).ToVector4(), "id");

			bgfx::setState(BGFX_STATE_DEFAULT);
			bgfx::submit(VIEW_ID_PASS, id_shader.programHandle);
		}
	}
	

	Vector4 cam_params(frameinfo.nearclip, frameinfo.farclip, (float)resolution.x, (float)resolution.y);
	//================== OUTLINE PASS ========================//
	InitPP(outline_shader, VIEW_OUTLINE_SELECTED_PASS);
	outline_shader.ApplyTextureOverride(gbuffer_selected.m_gbufferNormal, "tex_normal", 0);
	outline_shader.ApplyTextureOverride(gbuffer_selected.m_gbufferDepth, "tex_depth", 1);
	Vector4 outline_params(0.09, 0.01f, 0, 0);
	outline_shader.ApplyOverride(cam_params, "u_camera_params");
	outline_shader.ApplyOverride(outline_params, "u_outline_params");
	SubmitPP();
	//================== SSAO CALCULATION PASS ========================//
	InitPP(ssao_shader, VIEW_SSAO_PASS);
	curppshader.ApplyTextureOverride(gbuffer_all.m_gbufferNormal,"tex_normal",0);
	curppshader.ApplyTextureOverride(gbuffer_all.m_gbufferDepth,"tex_depth",1);
	curppshader.ApplyOverrideArray(m_ssao_kernel_data.data(), "u_kernel", 64);
	Vector4 ssao_params(0.09, 0.01f, 0, 0);
	curppshader.ApplyOverride(cam_params, "u_camera_params");
	curppshader.ApplyOverride(ssao_params, "u_ssao_params");
	SubmitPP();
	float blurrad = 9;
	float blur_sharpness = 5;
	InitPP(bilateralblur_shader, VIEW_BLUR0_PASS);
	curppshader.ApplyTextureOverride(m_pp_textures[VIEW_SSAO_PASS], "s_tex_input", 0);
	curppshader.ApplyTextureOverride(gbuffer_all.m_gbufferDepth, "s_tex_depth", 1);
	Vector4 blur_h_params(blurrad, blur_sharpness, 1.0f, 0.0f); // Direction: Horizontal (1,0)
	curppshader.ApplyOverride(cam_params, "u_camera_params");
	curppshader.ApplyOverride(blur_h_params, "u_blur_params");
	SubmitPP();
	InitPP(bilateralblur_shader, VIEW_BLUR1_PASS);
	curppshader.ApplyTextureOverride(m_pp_textures[VIEW_BLUR0_PASS], "s_tex_input", 0);
	curppshader.ApplyTextureOverride(gbuffer_all.m_gbufferDepth, "s_tex_depth", 1);
	Vector4 blur_v_params(blurrad, blur_sharpness, 0.0f, 1.0f);
	curppshader.ApplyOverride(cam_params, "u_camera_params");
	curppshader.ApplyOverride(blur_v_params, "u_blur_params");
	SubmitPP();

	TransferPPtoTexture(VIEW_BLUR1_PASS, m_ssaoTexture);

	//==================SHADOW PASS [VIEW_SHADOW_PASS]========================//
	for (size_t i = 0; i < max_lights; i++)
	{
		if (i >= available_views.size())
			break;

		bgfx::ViewId shadowViewId = available_views[i];

		if (i == frameinfo.lightdatas.size())
			break;

		const auto& light = frameinfo.lightdatas[i];

		light->UpdateContext(frameinfo.viewmat, frameinfo.projmat_lightusage);
		Matrix4 lightViewMat = light->GetViewMatrix();
		Matrix4 lightProjMat = light->GetProjectionMatrix();
		
		bgfx::setViewFrameBuffer(shadowViewId, m_shadowbuffers[i]);

		bgfx::setViewRect(shadowViewId, 0, 0, shadow_map_resolution, shadow_map_resolution);
		bgfx::setViewClear(shadowViewId, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);

		bgfx::setViewTransform(shadowViewId, (const float*)&lightViewMat, (const float*)&lightProjMat);
		for (const auto& shaderset : passinfo.callgroups)
		{
			if (shaderset.second.size() == 0)
				continue;

			drawbatch(shaderset.first, 0);

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
	bgfx::setViewClear(VIEW_SCENE_PASS, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x181818ff, 1.0f, 0);
	bgfx::setViewTransform(VIEW_SCENE_PASS, (const float*)&frameinfo.viewmat, (const float*)&frameinfo.projmat);

	//===============LINE PASS [VIEW_LINE_PASS]
	if (passinfo.lines_this_frame.size() > 0) {

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
		bgfx::submit(VIEW_SCENE_PASS, lineshader.programHandle);

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
		bgfx::submit(VIEW_SCENE_PASS, skyboxshader.programHandle);
	}
	//=================END OF SKYBOX PASS

	for (size_t i = 0; i < max_lights; i++)
	{
		// The original code was updating light uniforms for max_lights, even if not present,
		// which is necessary for uniform arrays in the shader.
		if (i >= frameinfo.lightdatas.size())
		{
			light_color_arr[i] = Vector4(0);
			light_range_arr[i] = 0;
			light_type_arr[i] = 0;
			continue;
		}

		const auto& light = frameinfo.lightdatas[i];

		light_color_arr[i] = Vector4(light->color, 1);
		light_view_arr[i] = light->GetViewMatrix();
		light_proj_arr[i] = light->GetProjectionMatrix();
		light_pos_arr[i] = Vector4(light->position, 1);
		light_type_arr[i].x = light->type;
		light_is_square_arr[i].x = light->square_project;
		light_nearclip_arr[i].x = light->near_clip;
		light_range_arr[i].x = light->range;
		light_bias_min_arr[i].x = light->bias_min;
		light_bias_mult_arr[i].x = light->bias_mult;
		light_cone_inner_arr[i].x = light->angle_inner;
		light_cone_outer_arr[i].x = light->angle_outer;
	}

	for (const auto& shaderset : passinfo.callgroups)
	{
		if (shaderset.second.size() == 0)
			continue;

		const auto& drawcall = shaderset.first;
		const auto& currentshaderdata = drawcall->get_shaderdata();

		drawcall->SyncMaterialData();

		// Set light data uniforms
		
		bgfx::setTexture(0, m_shadowArraySampler, m_shadowArrayTexture);
		drawcall->ApplyTextureOverride(m_ssaoTexture, "tex_ao", 4); // SSAO map bound to slot 4

		drawcall->ApplyOverrideArray<Matrix4>(light_view_arr.data(), "light_view", max_lights);
		drawcall->ApplyOverrideArray<Matrix4>(light_proj_arr.data(), "light_proj", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_color_arr.data(), "light_color", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_pos_arr.data(), "light_pos", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_type_arr.data(), "light_type", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_is_square_arr.data(), "light_is_square", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_nearclip_arr.data(), "light_nearclip", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_range_arr.data(), "light_range", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_bias_min_arr.data(), "light_bias_min", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_bias_mult_arr.data(), "light_bias_mult", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_cone_inner_arr.data(), "light_cone_inner", max_lights);
		drawcall->ApplyOverrideArray<Vector4>(light_cone_outer_arr.data(), "light_cone_outer", max_lights);

		drawbatch(shaderset.first, 0);

		bgfx::setState(BGFX_STATE_DEFAULT);
		bgfx::submit(VIEW_SCENE_PASS, currentshaderdata->programHandle);
	}

	//============================== BLEND PASS =============================//

	InitPP(bufferblend_shader, VIEW_SCREEN_PASS);
	curppshader.ApplyTextureOverride(mainscene_buffer.m_mainColorTexture, "tex_buffer_a", 0);
	curppshader.ApplyTextureOverride(m_pp_textures[VIEW_OUTLINE_SELECTED_PASS], "tex_buffer_b", 1);
	Vector4 blend_params(0, 0, 0, 0);
	curppshader.ApplyOverride(blend_params, "u_blend_params");
	SubmitPP();

	//============================== DEBUG CALLS =============================//

	//BLIT AREA AROUND CURSOR FOR ID
	
	Vector2Int from = frameinfo.pointer_pixelpos - Vector2Int(id_texture_size / 2, id_texture_size / 2);
	from.x = std::max(0, std::min(from.x, (int)resolution.x - id_texture_size));
	from.y = std::max(0, std::min(from.y, (int)resolution.y - id_texture_size));

	bgfx::blit(
		VIEW_DEBUG_BLITTER,
		localarea_id_texture_cpu.textureHandle, // Destination: Singular 2D texture
		0, 0, 0, 0,               // Mip 0, X 0, Y 0, Z 0
		id_buffer.m_mainColorTexture.textureHandle,     // Source: Your Texture Array
		0, from.x, from.y, 0,               // Mip 0, X 0, Y 0, Layer 'i'
		id_texture_size,
		id_texture_size
	);

	if (read_done) {
		this->frameid_readdone = bgfx::readTexture(localarea_id_texture_cpu.textureHandle, localarea_id_data.data());
		read_done = false;
	}

	if (passinfo.frame_id == this->frameid_readdone)
	{
		read_done = true;

		std::map<uint32_t, uint32_t> ids;  // This contains all the IDs found in the buffer
		uint32_t maxAmount = 0;
		for (auto& pixel : localarea_id_data)
		{
			if (0 == (pixel.r | pixel.g | pixel.b)) // Skip background
			{
				continue;
			}

			std::map<uint32_t, uint32_t>::iterator mapIter = ids.find(pixel.hashed());
			uint32_t amount = 1;
			if (mapIter != ids.end())
			{
				amount = mapIter->second + 1;
			}

			ids[pixel.hashed()] = amount; // Amount of times this ID (color) has been clicked on in buffer
			maxAmount = maxAmount > amount? maxAmount : amount;
		}

		this->current_id_onpointer = UINT32_MAX;
		if (maxAmount)
		{
			for (std::map<uint32_t, uint32_t>::iterator mapIter = ids.begin(); mapIter != ids.end(); mapIter++)
			{
				if (mapIter->second == maxAmount)
				{
					current_id_onpointer = mapIter->first;
					break;
				}
			}
		}
	}

	//BLIT SHADOW PASSES
	for (uint16_t i = 0; i < (uint16_t)max_lights; ++i) {
		// blit(viewId, dst, dstMip, dstX, dstY, dstZ, src, srcMip, srcX, srcY, srcZ, width, height, depth)
		bgfx::blit(
			VIEW_DEBUG_BLITTER,           // The view that performs the copy
			m_debugShadowTextures[i].textureHandle, // Destination: Singular 2D texture
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
	resolution = reso;

	//TODO: Destroy previous buffers

	uint16_t w = (uint16_t)reso.x;
	uint16_t h = (uint16_t)reso.y;

	//PINGPONGING
	RegisterPPFramebuffer(VIEW_SSAO_PASS);
	RegisterPPFramebuffer(VIEW_BLUR0_PASS);
	RegisterPPFramebuffer(VIEW_BLUR1_PASS);
	RegisterPPFramebuffer(VIEW_OUTLINE_SELECTED_PASS, bgfx::TextureFormat::BGRA8);
	RegisterPPFramebuffer(VIEW_SCREEN_PASS, bgfx::TextureFormat::BGRA8);
	TextureLoader::RegisterExternal("OUTLINE_PASS", m_pp_textures[VIEW_OUTLINE_SELECTED_PASS]);

	//GBUFFERS
	gbuffer_all = GBuffer(w, h, "ALL", VIEW_GBUFFER_PASS);
	gbuffer_selected = GBuffer(w, h, "SELECTED", VIEW_GBUFFER_SELECTED_PASS);

	//SSAO
	m_ssaoTexture = TextureData{
		.textureHandle = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_BLIT_DST)
	};
	TextureLoader::RegisterExternal("m_ssaoTexture", m_ssaoTexture);

	//MAIN PASS
	mainscene_buffer = ColorDepthBuffer(w, h, "MAINPASS", VIEW_SCENE_PASS);

	//ID PASS
	id_buffer = ColorDepthBuffer(w, h, "IDPASS", VIEW_ID_PASS);
	localarea_id_texture_cpu = TextureData{
		.textureHandle = bgfx::createTexture2D(id_texture_size, id_texture_size, false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK)
	};
	TextureLoader::RegisterExternal("localarea_id_texture", localarea_id_texture_cpu);

	return m_pp_textures[VIEW_SCREEN_PASS];
}
