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
				Vector3 tangent;

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