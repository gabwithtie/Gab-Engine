#include "MaterialLoader.h"

using namespace gbe;
using namespace gfx;

MaterialData MaterialLoader::LoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, asset::data::MaterialLoadData* data) {
	data->shader = asset::Shader::GetAssetById(importdata.shader);

	const auto FindImportedOverride = [&](const std::string& id) -> const asset::data::MaterialOverrideImport* {
		for (const auto& ovr : importdata.overrides)
		{
			if (ovr.id == id)
				return &ovr;
		}
		return nullptr;
		};

	//Default overrides
	for (const auto& field : ShaderLoader::GetAssetData(data->shader).uniformfields)
	{
		auto importedoverride = FindImportedOverride(field.name);

		switch (field.type)
		{
		case asset::Shader::UniformFieldType::BOOL:
			if (importedoverride != nullptr)
				asset->setOverride(field.name, importedoverride->value_single != 0);
			else
			asset->setOverride(field.name, false);
			break;
		case asset::Shader::UniformFieldType::FLOAT:
			if (importedoverride != nullptr)
				asset->setOverride(field.name, importedoverride->value_single);
			else
				asset->setOverride(field.name, 0.0f);
			break;
		case asset::Shader::UniformFieldType::INT :
			if (importedoverride != nullptr)
				asset->setOverride(field.name, importedoverride->value_single);
			else
				asset->setOverride(field.name, 0);
			break;
		case asset::Shader::UniformFieldType::VEC2 :
			if (importedoverride != nullptr)
				asset->setOverride(field.name, Vector2(importedoverride->value_vec2[0], importedoverride->value_vec2[1]));
			else
				asset->setOverride(field.name, Vector2::zero);
			break;
		case asset::Shader::UniformFieldType::VEC3 :
			if (importedoverride != nullptr)
				asset->setOverride(field.name, Vector3(importedoverride->value_vec3[0], importedoverride->value_vec3[1], importedoverride->value_vec3[2]));
			else
				asset->setOverride(field.name, Vector3::zero);
			break;
		case asset::Shader::UniformFieldType::VEC4 :
			if (importedoverride != nullptr)
				asset->setOverride(field.name, Vector4(importedoverride->value_vec4[0], importedoverride->value_vec4[1], importedoverride->value_vec4[2], importedoverride->value_vec4[3]));
			else
				asset->setOverride(field.name, Vector4(0, 0, 0, 1));
			break;
		case asset::Shader::UniformFieldType::MAT4 :
			asset->setOverride(field.name, Matrix4(1.0f));
			break;
		case asset::Shader::UniformFieldType::TEXTURE:
			if (importedoverride != nullptr)
				asset->setOverride(field.name, asset::Texture::GetAssetById(importedoverride->value_tex));
			break;
		}
	}

	return MaterialData{
		//NONE
	};
}

void MaterialLoader::UnLoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, asset::data::MaterialLoadData* data) {

}

