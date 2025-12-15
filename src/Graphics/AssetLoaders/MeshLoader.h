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
		// TransformUBO removed as it is typically managed by RenderPipeline/DrawCall in a bgfx context

		struct MeshData {
			asset::data::MeshLoadData* loaddata;

			// Replaced vulkan::Buffer* with bgfx handles
			bgfx::VertexBufferHandle vertex_vbh = BGFX_INVALID_HANDLE;
			bgfx::IndexBufferHandle index_vbh = BGFX_INVALID_HANDLE;
		};

		class MeshLoader : public asset::AssetLoader<asset::Mesh, asset::data::MeshImportData, asset::data::MeshLoadData, MeshData> {
		protected:
			MeshData LoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data) override;
			void UnLoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data) override;
		};
	}
}