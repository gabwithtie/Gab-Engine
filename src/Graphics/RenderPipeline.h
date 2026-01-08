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
#include "Renderer.h"

namespace gbe {

	using namespace gfx;
	class Editor;


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

		//============RUNTIME=======================//
		DrawCall* default_drawcall;

		GraphicsRenderInfo currentrenderinfo;

		//============DYNAMICALLY ALLOCATED=======================//
		Renderer* cur_renderer;

		bool handled_resolution_change = true;

		void ReloadFrame(); // BGFX function to create frame buffers/textures

	public:
		RenderPipeline(gbe::Window&, Vector2Int);
		~RenderPipeline(); // Must destroy bgfx handles
		static DrawCall* RegisterDrawCall(asset::Mesh* mesh, asset::Material* material);
		static DrawCall* RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material);
		static DrawCall* GetDefaultDrawCall();
		static void DrawLine(Vector3 a, Vector3 b);

		inline void InitializeAssetRequisites() {
			this->cur_renderer->InitializeAssetRequests();
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

		void RenderFrame(const SceneRenderInfo& frameinfo);

		gbe::Matrix4* RegisterInstance(void* instance_id, DrawCall* drawcall, gbe::Matrix4 matrix, int rendergroup = 0);
		static void UnRegisterInstance(void* instance_id, int rendergroup = 0);
	};
}