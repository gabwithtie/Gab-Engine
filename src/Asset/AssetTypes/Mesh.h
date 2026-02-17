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
		}

		class Mesh : public BaseAsset<Mesh, data::MeshImportData> {
		public:
			Mesh(std::filesystem::path path);
		};
	}
}