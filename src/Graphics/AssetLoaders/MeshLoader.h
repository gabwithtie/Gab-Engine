#pragma once

#include "../tiny_obj_loader.h"
#include "Asset/gbe_asset.h"
#include "Math/gbe_math.h"

#include "TextureLoader.h"
#include <optional>
#include <tuple>

namespace gbe {
	namespace gfx {
		struct TransformUBO {
			gbe::Matrix4 model;
			gbe::Matrix4 view;
			gbe::Matrix4 proj;
		};

		struct MeshData {
			asset::data::MeshLoadData* loaddata;

			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;
			VkBuffer indexBuffer;
			VkDeviceMemory indexBufferMemory;
		};

		class MeshLoader : public asset::AssetLoader<asset::Mesh, asset::data::MeshImportData, asset::data::MeshLoadData, MeshData> {
		private:
			VkDevice* vkdevice;
			VkPhysicalDevice* vkphysicaldevice;
		protected:
			MeshData LoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data) override;
			void UnLoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data) override;
		public:
			void PassDependencies(VkDevice* vkdevice, VkPhysicalDevice* vkphysicaldevice);
		};
	}
}