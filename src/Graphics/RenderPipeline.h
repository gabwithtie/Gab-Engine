#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>


#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <string>

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
		Vector2Int resolution;

		//Loaders
		ShaderLoader shaderloader;
		MeshLoader meshloader;
		TextureLoader textureloader;
		MaterialLoader materialloader;

		VkImage* mostrecent_screenshot = nullptr;

		//============RUNTIME=======================//
		DrawCall* default_drawcall;
		std::vector<DrawCall*> drawcalls;

		std::unordered_map<void*, std::map<int, CallInstance>> calls;
		std::map<int, std::vector<void*>> sortedcalls;
		
		//============DYNAMICALLY ALLOCATED=======================//
		vulkan::Instance* vulkanInstance;
		vulkan::ForwardRenderer* renderer; //owned by vulkanInstance
		
	public:
		struct FrameRenderInfo {
			//Camera info
			Vector3 camera_pos;
			Matrix4 viewmat;
			Matrix4 projmat;
			Matrix4 projmat_lightusage;
			float nearclip;
			float farclip;
			
			//Environment info
			std::vector<gfx::Light*> lightdatas;
		};

		static RenderPipeline* Get_Instance();
		
		RenderPipeline(gbe::Window&, Vector2Int);
		static DrawCall* RegisterDrawCall(asset::Mesh* mesh, asset::Material* material);
		static DrawCall* RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material);
		static DrawCall* GetDefaultDrawCall();

		void AssignEditor(Editor* editor);
		void SetCameraShader(asset::Shader* postprocess);

		void SetResolution(Vector2Int newresolution);
		
		void RenderFrame(const FrameRenderInfo& frameinfo);
		std::vector<unsigned char> ScreenShot(bool write_file = false);

		Matrix4* RegisterCall(void* instance_id, DrawCall* drawcall, Matrix4 matrix, int order = 0);
		static void UnRegisterCall(void* instance_id);
	};
}