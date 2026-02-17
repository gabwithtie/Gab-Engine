#pragma once

#include "Asset/gbe_asset.h"
#include "ShaderLoader.h"

namespace gbe {
	namespace gfx {
		struct MaterialOverride {

			asset::Shader::UniformFieldType type;

			bool value_bool;
			float value_float;
			Vector2 value_vec2;
			Vector3 value_vec3;
			Vector4 value_vec4;
			Matrix4 value_mat4;
			asset::Texture* value_tex;
			int tex_stage;
		};

		struct MaterialData {
			std::unordered_map<std::string, MaterialOverride> overrides;
			bool shadowcaster;
			int defaultrendergroup;
			asset::Shader* shader = nullptr;

			size_t getOverrideCount() const {
				return this->overrides.size();
			}

			MaterialOverride& getOverride(size_t index, std::string& id) {
				auto it = this->overrides.begin();
				std::advance(it, index);

				id = it->first;
				return it->second;
			}

			template <typename TValue>
			void setOverride(std::string id, TValue value) {}

			template<>
			void setOverride<bool>(std::string id, bool value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::BOOL;
				materialOverride.value_bool = value;

				this->overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<float>(std::string id, float value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::FLOAT;
				materialOverride.value_float = value;

				this->overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Vector2>(std::string id, Vector2 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::VEC2;
				materialOverride.value_vec2 = value;

				this->overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Vector3>(std::string id, Vector3 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::VEC3;
				materialOverride.value_vec3 = value;

				this->overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Vector4>(std::string id, Vector4 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::VEC4;
				materialOverride.value_vec4 = value;

				this->overrides.insert_or_assign(id, materialOverride);
			}
			template<>
			void setOverride<Matrix4>(std::string id, Matrix4 value) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::MAT4;
				materialOverride.value_mat4 = value;

				this->overrides.insert_or_assign(id, materialOverride);
			}

			void setTextureOverride(std::string id, asset::Texture* value, int stage) {
				auto materialOverride = MaterialOverride();
				materialOverride.type = asset::Shader::UniformFieldType::TEXTURE;
				materialOverride.value_tex = value;
				materialOverride.tex_stage = stage;

				this->overrides.insert_or_assign(id, materialOverride);
			}
		};

		class MaterialLoader : public asset::AssetLoader<asset::Material, asset::data::MaterialImportData, MaterialData > {
		protected:
			void LoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, MaterialData* data) override;
			void UnLoadAsset_(MaterialData* data) override;
			inline virtual void OnAsyncTaskCompleted(AsyncLoadTask* loadtask) override {
				//This is a synchronous loader
			}
		public:
			inline virtual void AssignSelfAsLoader() override {
				AssetLoader::AssignSelfAsLoader();
				asset::all_asset_loaders.insert_or_assign(asset::MATERIAL, this);
			}
		};
	}
}