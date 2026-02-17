#pragma once

#include "Asset/BaseAsset.h"
#include <functional>
#include "Asset/AssetTypes/Texture.h"
#include "Math/gbe_math.h"

namespace gbe {
	namespace asset {
		namespace data {
			struct ShaderImportData {
				std::string vert;
				std::string frag;
				std::string def;
				std::string wireframe;
				std::string line;
				std::string renderpass;
				std::string backfacecull;
			};
		}

		class Shader : public BaseAsset<Shader, data::ShaderImportData> {
		public:
			enum UniformFieldType {
				BOOL,
				INT,
				FLOAT,
				VEC2,
				VEC3,
				VEC4,
				MAT4,
				TEXTURE,
			};

			Shader(std::filesystem::path path);
		};
	}
}