#include "MaterialLoader.h"

using namespace gbe;
using namespace gfx;

MaterialData MaterialLoader::LoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, asset::data::MaterialLoadData* data) {
	data->shader = asset::Shader::GetAssetById(importdata.shader);
	data->shadowcaster = importdata.shadowcaster != 0;
	data->defaultrendergroup = importdata.defaultrendergroup;

	//Default overrides
	for (const auto& importedoverride : importdata.overrides)
	{
		if (importedoverride.type == "single")
			asset->setOverride(importedoverride.id, importedoverride.value_single);
		if (importedoverride.type == "vec2")
			asset->setOverride(importedoverride.id, Vector2(importedoverride.value_vec2[0], importedoverride.value_vec2[1]));
		if (importedoverride.type == "vec3")
			asset->setOverride(importedoverride.id, Vector3(importedoverride.value_vec3[0], importedoverride.value_vec3[1], importedoverride.value_vec3[2]));
		if (importedoverride.type == "vec4")
			asset->setOverride(importedoverride.id, Vector4(importedoverride.value_vec4[0], importedoverride.value_vec4[1], importedoverride.value_vec4[2], importedoverride.value_vec4[3]));
		if (importedoverride.type == "texture")
			asset->setTextureOverride(importedoverride.id, asset::Texture::GetAssetById(importedoverride.value_tex), importedoverride.tex_stage);
	}

	return MaterialData{
		//NONE
	};
}

void MaterialLoader::UnLoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, asset::data::MaterialLoadData* data) {

}

