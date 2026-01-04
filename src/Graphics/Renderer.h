#pragma once

#include "Math/gbe_math.h"
#include "Asset/gbe_asset.h"

#include "Data/Light.h"
#include "Data/DrawCall.h"

namespace gbe {
	namespace gfx {
		struct SceneRenderInfo {
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

		struct GraphicsRenderInfo {
			std::unordered_map<void*, gbe::Matrix4> matrix_map;
			std::unordered_map<int, std::unordered_map<DrawCall*, std::vector<void*>>> sortedcalls;

			//LINES
			const size_t max_lines = 1000;
			std::vector<asset::data::Vertex> lines_this_frame;
		};

		class Renderer {
		public:
			inline Renderer() {

			}
			virtual TextureData ReloadFrame(Vector2Int reso) = 0;
			virtual void RenderFrame(const SceneRenderInfo& frameinfo, GraphicsRenderInfo& passinfo) = 0;
			virtual void InitializeAssetRequests() = 0;
		};
	}
}