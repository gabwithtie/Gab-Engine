#pragma once

#include "Math/gbe_math.h"
#include "Asset/gbe_asset.h"

#include "Data/Light.h"
#include "Data/DrawCall.h"

#include <queue>

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
			std::vector<gbe::gfx::Vertex> lines_this_frame;
		};

		class Renderer {
		public:
			enum CPU_PASS_MODE {
				PASS_ID,
				PASS_UV
			};

			struct BRGA_t {
				uint8_t b;
				uint8_t g;
				uint8_t r;
				uint8_t a;

				BRGA_t() : b(0), g(0), r(0), a(0) {}
				uint32_t hashed() const {
					return (uint32_t(b) << 0) | (uint32_t(g) << 8) | (uint32_t(r) << 16);
				}
				BRGA_t(uint32_t hash) {
					b = (hash >> 0) & 0xFF;
					g = (hash >> 8) & 0xFF;
					r = (hash >> 16) & 0xFF;
				}
				Vector4 ToVector4() const {
					return Vector4(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
				}
			};

			struct CpuDataResponse;

			struct CpuDataRequest {
				std::string override_id = "";
				CPU_PASS_MODE cpu_pass_mode;
				Vector2Int cursor_pixel_pos;
				int rect_size = 2;
				std::function<void(CpuDataResponse&)> callback;
			};
			struct CpuDataResponse {
				CpuDataRequest request = {};

				std::vector<BRGA_t> cpu_data;
				TextureData render_target;

				uint32_t frame_done = UINT32_MAX;
				bool passed = false;
				bool received = false;
			};

		protected:
			std::vector<CpuDataResponse> cpu_data_responses;
		public:
			inline void SubmitCpuDataRequest(CpuDataRequest request) {
				if(request.override_id.size() > 0)
					for (const auto& req : cpu_data_responses)
					{
						if (req.request.override_id == request.override_id) {
							return;
						}
					}

				cpu_data_responses.push_back(PreprocessCpuRequest(request));
			}

			inline Renderer() {

			}

			virtual CpuDataResponse PreprocessCpuRequest(CpuDataRequest request) = 0;
			virtual TextureData ReloadFrame(Vector2Int reso) = 0;
			virtual void RenderFrame(const SceneRenderInfo& frameinfo, GraphicsRenderInfo& passinfo) = 0;
			virtual void InitializeAssetRequests() = 0;
		};
	}
}