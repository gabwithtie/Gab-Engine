#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "Texture.h"
#include "Shader.h"
#include "Math/gbe_math.h"

namespace gbe {
	namespace asset {
		namespace data {
			struct MaterialOverrideImport {
				std::string id;
				std::string type;
				bool value_bool;
				float value_single;
				float value_vec2[2];
				float value_vec3[3];
				float value_vec4[4];
				std::string value_tex;
				int tex_stage;
			};

			struct MaterialImportData {
				std::string shader;
				int shadowcaster;
				int defaultrendergroup;
				std::vector<MaterialOverrideImport> overrides;
			};
		}

		class Material : public BaseAsset<asset::Material, data::MaterialImportData> {
		public:
			Material(std::filesystem::path path);
		};
	}
}