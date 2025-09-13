#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "Texture.h"
#include "Shader.h"
#include "Math/gbe_math.h"

namespace gbe {
	namespace asset {
		class MaterialOverride {
		public:
			asset::Shader::UniformFieldType type;

			MaterialOverride();
			~MaterialOverride();

			bool value_bool;
			float value_float;
			Vector2 value_vec2;
			Vector3 value_vec3;
			Vector4 value_vec4;
			Matrix4 value_mat4;
			asset::Texture* value_tex;

			bool registered_change = false;
		};

		namespace data {
			struct MaterialOverrideImport {
				std::string id;
				bool value_bool;
				float value_single;
				float value_vec2[2];
				float value_vec3[3];
				float value_vec4[4];
				std::string value_tex;
			};

			struct MaterialImportData {
				std::string shader;

				std::vector<MaterialOverrideImport> overrides;
			};
			struct MaterialLoadData {
				std::unordered_map<std::string, MaterialOverride> overrides;
				asset::Shader* shader = nullptr;
			};
		}

		class Material : public BaseAsset<asset::Material, data::MaterialImportData, data::MaterialLoadData> {
		public:
			Material(std::filesystem::path path);

			size_t getOverrideCount() const {
				return this->load_data.overrides.size();
			}

			MaterialOverride& getOverride(size_t index, std::string& id) {
				auto it = this->load_data.overrides.begin();
				std::advance(it, index);

				id = it->first;
				return it->second;
			}

			template <typename TValue>
			void setOverride(std::string id, TValue value) {}

			template<>
			void setOverride<bool>(std::string id, bool value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::BOOL;
				materialOverride.value_bool = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<float>(std::string id, float value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::FLOAT;
				materialOverride.value_float = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Vector2>(std::string id, Vector2 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::VEC2;
				materialOverride.value_vec2 = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Vector3>(std::string id, Vector3 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::VEC3;
				materialOverride.value_vec3 = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Vector4>(std::string id, Vector4 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::VEC4;
				materialOverride.value_vec4 = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Matrix4>(std::string id, Matrix4 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::MAT4;
				materialOverride.value_mat4 = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<asset::Texture*>(std::string id, asset::Texture* value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = Shader::UniformFieldType::TEXTURE;
				materialOverride.value_tex = value;

				this->load_data.overrides.insert_or_assign(id, materialOverride);
			}
		};
	}
}