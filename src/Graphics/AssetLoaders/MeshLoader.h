#pragma once

#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

#include "TextureLoader.h"
#include <optional>
#include <tuple>

// Replaced Vulkan objects with bgfx
#include <bgfx/bgfx.h> 

namespace gbe {
	namespace gfx {
		extern bgfx::VertexLayout s_VERTEXLAYOUT;

		struct Vertex {
			Vector3 pos;
			Vector3 normal;
			Vector3 color;
			Vector2 texCoord;
			Vector4 tangent;

			// Comparison operator for deduplication
			inline bool operator<(const Vertex& other) const {
				// A simple way to compare is to compare the members in a defined order.
				// This is necessary to use Vertex as a key in std::map.
				if (pos.x != other.pos.x) return pos.x < other.pos.x;
				if (pos.y != other.pos.y) return pos.y < other.pos.y;
				if (pos.z != other.pos.z) return pos.z < other.pos.z;

				if (normal.x != other.normal.x) return normal.x < other.normal.x;
				if (normal.y != other.normal.y) return normal.y < other.normal.y;
				if (normal.z != other.normal.z) return normal.z < other.normal.z;

				// You can add more comparisons for color, texCoord, etc.
				if (texCoord.x != other.texCoord.x) return texCoord.x < other.texCoord.x;
				if (texCoord.y != other.texCoord.y) return texCoord.y < other.texCoord.y;

				// Add more comparisons if needed for other attributes
				return false;
			}
		};

		struct MeshData {
			// Replaced vulkan::Buffer* with bgfx handles
			bgfx::VertexBufferHandle vertex_vbh = BGFX_INVALID_HANDLE;
			bgfx::IndexBufferHandle index_vbh = BGFX_INVALID_HANDLE;

			std::vector<Vertex> vertices;
			std::vector<uint16_t> indices;
			std::vector<std::vector<uint16_t>> faces;
		};

		class MeshLoader : public asset::AssetLoader<asset::Mesh, asset::data::MeshImportData, MeshData> {
		public:
			struct AsyncMeshTask : public MeshLoader::AsyncLoadTask {
				std::vector<Vertex> out_vertices;
				std::vector<uint16_t> out_indices;
				std::vector<std::vector<uint16_t>> out_faces;
			};
			std::size_t MaxAsyncTasks = 16;

		protected:
			void LoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, MeshData* data) override;
			void UnLoadAsset_(MeshData* data) override;
			virtual void OnAsyncTaskCompleted(MeshLoader::AsyncLoadTask* loadtask) override;
		public:
			void AssignSelfAsLoader() override;
		};
	}
}