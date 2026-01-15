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
			Vector2Int pointer_pixelpos;
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
			struct InstanceInfo {
				gbe::Matrix4 transform;
				DrawCall* drawcall;
				std::unordered_map<int, bool> rendergroups;
				bool enabled = true;
			};
			std::unordered_map<uint32_t, InstanceInfo> infomap;
			std::unordered_map<DrawCall*, std::vector<uint32_t>> callgroups;

			//LINES
			uint32_t frame_id = 0;
			const size_t max_lines = 1000;
			std::vector<asset::data::Vertex> lines_this_frame;
		};

		class Renderer {
		protected:
			uint32_t current_id_onpointer;
		public:
			inline Renderer() {

			}
			inline uint32_t GetCurrentIdOnPointer() {
				return current_id_onpointer;
			}

			virtual TextureData ReloadFrame(Vector2Int reso) = 0;
			virtual void RenderFrame(const SceneRenderInfo& frameinfo, GraphicsRenderInfo& passinfo) = 0;
			virtual void InitializeAssetRequests() = 0;
		};
	}
}