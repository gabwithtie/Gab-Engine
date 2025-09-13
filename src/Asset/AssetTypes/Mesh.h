#pragma once

#include "Asset/BaseAsset.h"

#include "Math/gbe_math.h"
#include <filesystem>

namespace gbe {
	namespace asset {

		namespace data {
			struct MeshImportData {
				std::string path;
			};
			struct Vertex {
				Vector3 pos;
				Vector3 normal;
				Vector3 color;
				Vector2 texCoord;
			};
			struct MeshLoadData {
				std::vector<Vertex> vertices;
				std::vector<uint16_t> indices;
				std::vector<std::vector<uint16_t>> faces;
			};
		}

		class Mesh : public BaseAsset<Mesh, data::MeshImportData, data::MeshLoadData> {
		public:
			Mesh(std::filesystem::path path);
		};
	}
}