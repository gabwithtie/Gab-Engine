#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>


#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_map>

#include <sstream>
#include <functional>

// BGFX: Include the library and its platform-specific initialization header
#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include "Math/gbe_math.h"
#include "Asset/gbe_asset.h"

#include "Data/DrawCall.h"
#include "AssetLoaders/TextureLoader.h"
#include "AssetLoaders/ShaderLoader.h"
#include "AssetLoaders/MeshLoader.h"
#include "AssetLoaders/MaterialLoader.h"
#include "Window/gbe_window.h"
#include "Data/CallInstance.h"
#include "Data/Light.h"


namespace cv {
	class Mat;
}

namespace gbe {
	using namespace gfx;
	class Editor;

	// BGFX: Define the View IDs for each rendering pass
	enum RenderViewId
	{
		VIEW_SHADOW_PASS = 0,
		VIEW_MAIN_PASS = 1,
		VIEW_LINE_PASS = 2,
		VIEW_SKYBOX_PASS = 3,
		VIEW_EDITOR_PASS = 4,

		// Must be the last one
		VIEW_COUNT
	};


	class RenderPipeline {
	private:
		static RenderPipeline* Instance;

		Window& window;
		Editor* editor;
		Vector2Int screen_resolution;
		Vector2Int viewport_resolution;

		//Loaders
		ShaderLoader shaderloader;
		MeshLoader meshloader;
		TextureLoader textureloader;
		MaterialLoader materialloader;

		// BGFX: Main View ID for the final render to screen
		bgfx::ViewId m_mainViewId = VIEW_MAIN_PASS;

		// BGFX: Render target handle for the main pass (the color buffer for the final scene)
		bgfx::FrameBufferHandle m_mainPassFBO = BGFX_INVALID_HANDLE;
		// BGFX: Render target handle for the shadow pass (depth/color buffer)
		bgfx::FrameBufferHandle m_shadowPassFBO = BGFX_INVALID_HANDLE;

		//============RUNTIME=======================//
		DrawCall* default_drawcall;

		std::unordered_map<void*, gbe::Matrix4> matrix_map;
		std::unordered_map<int, std::unordered_map<DrawCall*, std::vector<void*>>> sortedcalls;

		//LINES
		const size_t maxlines = 1000;
		std::vector<asset::data::Vertex> lines_this_frame;
		DrawCall* line_call;
		DrawCall* skybox_call;

		//============DYNAMICALLY ALLOCATED=======================//
		// BGFX: Vertex buffer for lines
		bgfx::DynamicVertexBufferHandle m_line_vbh = BGFX_INVALID_HANDLE;

		// BGFX: Textures used as attachments for the Frame Buffers
		bgfx::TextureHandle m_mainColorTexture = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle m_mainDepthTexture = BGFX_INVALID_HANDLE;
		bgfx::TextureHandle m_shadowDepthTexture = BGFX_INVALID_HANDLE;

		bool handled_resolution_change = true;

		void UpdateReferences();
		void CreateFrameBuffers(); // BGFX function to create frame buffers/textures

	public:
		struct FrameRenderInfo {
			//Camera info
			Vector3 camera_pos;
			gbe::Matrix4 viewmat;
			gbe::Matrix4 projmat;
			gbe::Matrix4 projmat_lightusage;
			float nearclip;
			float farclip;
			bool skip_main_pass;

			//Environment info
			std::vector<gfx::Light*> lightdatas;
		};


		RenderPipeline(gbe::Window&, Vector2Int);
		~RenderPipeline(); // Must destroy bgfx handles
		void PrepareCall(DrawCall* drawcall);
		static DrawCall* RegisterDrawCall(asset::Mesh* mesh, asset::Material* material, int order = 0);
		static DrawCall* RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material);
		static DrawCall* GetDefaultDrawCall();
		static void DrawLine(Vector3 a, Vector3 b);

		inline void InitializeAssetRequisites() {
			//Register Line Drawcall
			this->line_call = this->RegisterDrawCall(nullptr, asset::Material::GetAssetById("line"), 0);
			this->PrepareCall(this->line_call);

			this->skybox_call = this->RegisterDrawCall(asset::Mesh::GetAssetById("cube"), asset::Material::GetAssetById("skybox_gradient"), 0);
			this->PrepareCall(this->skybox_call);
		}

		inline static RenderPipeline* Get_Instance() {
			return Instance;
		}
		inline void AssignEditor(Editor* editor) {
			this->editor = editor;
		}

		// BGFX: The resolution change logic is adapted for bgfx::reset() and re-creating FBOs
		inline static void SetScreenResolution(Vector2Int newresolution) {
			Instance->screen_resolution = newresolution;
			Instance->handled_resolution_change = false; // Trigger FBO/bgfx::reset later
		}
		inline static void SetViewportResolution(Vector2Int newresolution, Vector2Int offset) {
			Instance->viewport_resolution = newresolution;
			Instance->handled_resolution_change = false; // Trigger FBO/bgfx::reset later

			Instance->window.Set_viewport(
				{
					(float)newresolution.x / Instance->screen_resolution.x,
					(float)newresolution.y / Instance->screen_resolution.y
				},
				offset);
		}
		inline static Vector2Int GetViewportResolution() {
			return Instance->viewport_resolution;
		}
		inline static Vector2Int GetScreenResolution() {
			return Instance->screen_resolution;
		}


		void RenderFrame(const FrameRenderInfo& frameinfo);
		std::vector<unsigned char> ScreenShot(bool write_file = false); // Still complex due to reading back data

		gbe::Matrix4* RegisterInstance(void* instance_id, DrawCall* drawcall, gbe::Matrix4 matrix);
		static void UnRegisterCall(void* instance_id);
	};
}