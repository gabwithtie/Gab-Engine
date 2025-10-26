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

#include "Ext/GabVulkan/Objects.h"
#include "Ext/GabVulkan/Utility/ValidationLayers.h"
#include "Ext/GabVulkan/Utility/MemoryBarrier.h"
#include "Ext/GabVulkan/Components/ForwardRenderer/ForwardRenderer.h"

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

		VkImage* mostrecent_screenshot = nullptr;

		//============RUNTIME=======================//
		DrawCall* default_drawcall;

		std::unordered_map<void*, Matrix4> matrix_map;
		std::unordered_map<int, std::unordered_map<DrawCall*, std::vector<void*>>> sortedcalls;
		
		//LINES
		const size_t maxlines = 1000;
		std::vector<asset::data::Vertex> lines_this_frame;
		DrawCall* line_call;
		DrawCall* skybox_call;
		
		//============DYNAMICALLY ALLOCATED=======================//
		vulkan::Instance* vulkanInstance;
		vulkan::ForwardRenderer* renderer; //owned by vulkanInstance
		vulkan::Buffer* line_vertexBuffer = nullptr;

		bool handled_resolution_change = true;

		void UpdateReferences();
	public:
		struct FrameRenderInfo {
			//Camera info
			Vector3 camera_pos;
			Matrix4 viewmat;
			Matrix4 projmat;
			Matrix4 projmat_lightusage;
			float nearclip;
			float farclip;
			bool skip_main_pass;
			
			//Environment info
			std::vector<gfx::Light*> lightdatas;
		};

		
		RenderPipeline(gbe::Window&, Vector2Int);
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

		inline static void SetScreenResolution(Vector2Int newresolution) {
			Instance->screen_resolution = newresolution;
			Instance->handled_resolution_change = false;
		}
		inline static void SetViewportResolution(Vector2Int newresolution, Vector2Int offset) {
			Instance->viewport_resolution = newresolution;
			Instance->handled_resolution_change = false;

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
		std::vector<unsigned char> ScreenShot(bool write_file = false);

		Matrix4* RegisterInstance(void* instance_id, DrawCall* drawcall, Matrix4 matrix);
		static void UnRegisterCall(void* instance_id);
	};
}