#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
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

#include "Math/gbe_math.h"
#include "Asset/gbe_asset.h"

#include "Data/DrawCall.h"
#include "Data/Light.h"
#include "Data/Framebuffer.h"
#include "AssetLoaders/TextureLoader.h"
#include "AssetLoaders/ShaderLoader.h"
#include "AssetLoaders/MeshLoader.h"
#include "AssetLoaders/MaterialLoader.h"
#include "Window/gbe_window.h"
#include "Data/CallInstance.h"

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


		//CAPTURING
		VkImage* mostrecent_screenshot = nullptr;

		//============RUNTIME=======================//
		DrawCall* default_drawcall;
		std::vector<DrawCall*> drawcalls;
		std::list<gfx::Light*> lights_this_frame;
		const int maxlights = 10;

		std::unordered_map<void*, CallInstance> calls;
		std::map<int, std::vector<void*>> sortedcalls;
		
		//============DYNAMICALLY ALLOCATED=======================//
		vulkan::Instance* vulkanInstance;
		
	public:
		static RenderPipeline* Get_Instance();
		
		RenderPipeline(gbe::Window&, Vector2Int);
		DrawCall* RegisterDrawCall(asset::Mesh* mesh, asset::Material* material);
		DrawCall* RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material);
		DrawCall* GetDefaultDrawCall();

		void AssignEditor(Editor* editor);
		void SetCameraShader(asset::Shader* postprocess);
		bool TryPushLight(gfx::Light* data, bool priority = false);

		void SetResolution(Vector2Int newresolution);
		
		void RenderFrame(Matrix4 viewmat, Matrix4 projmat, float& nearclip, float& farclip);
		std::vector<unsigned char> ScreenShot(bool write_file = false);

		Matrix4* RegisterCall(void* instance_id, DrawCall* drawcall, Matrix4 matrix, int order = 0);
		void UnRegisterCall(void* instance_id);
	};
}