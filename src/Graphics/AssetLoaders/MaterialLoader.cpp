#include "MaterialLoader.h"
#include "Editor/gbe_editor.h"

using namespace gbe;
using namespace gfx;

MaterialData MaterialLoader::LoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, asset::data::MaterialLoadData* data) {
    data->shader = asset::Shader::GetAssetById(importdata.shader);
    data->shadowcaster = importdata.shadowcaster != 0;
    data->defaultrendergroup = importdata.defaultrendergroup;

    auto newinspectordata = new editor::InspectorData();

    for (const auto& importedoverride : importdata.overrides)
    {
        std::string id = importedoverride.id;

        // --- Float / Single ---
        if (importedoverride.type == "single") {
            asset->setOverride(id, importedoverride.value_single);

            auto field = new editor::InspectorFloat();
            field->name = id;
            field->getter = [asset, id]() { return asset->Get_load_data().overrides.at(id).value_float; };
            field->setter = [asset, id](float val) {
                asset->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) i.value_single = val;
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Vector 2 ---
        else if (importedoverride.type == "vec2") {
            asset->setOverride(id, Vector2(importedoverride.value_vec2[0], importedoverride.value_vec2[1]));

            auto field = new editor::InspectorField<Vector2>();
            field->name = id;
            field->getter = [asset, id]() { return asset->Get_load_data().overrides.at(id).value_vec2; };
            field->setter = [asset, id](Vector2 val) {
                asset->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) { i.value_vec2[0] = val.x; i.value_vec2[1] = val.y; }
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Vector 3 / Color ---
        else if (importedoverride.type == "vec3") {
            asset->setOverride(id, Vector3(importedoverride.value_vec3[0], importedoverride.value_vec3[1], importedoverride.value_vec3[2]));

            editor::InspectorField<Vector3>* field;
            if (id.find("Color") != std::string::npos || id.find("color") != std::string::npos)
                field = new editor::InspectorColor();
            else
                field = new editor::InspectorVec3();

            field->name = id;
            field->getter = [asset, id]() { return asset->Get_load_data().overrides.at(id).value_vec3; };
            field->setter = [asset, id](Vector3 val) {
                asset->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) { i.value_vec3[0] = val.x; i.value_vec3[1] = val.y; i.value_vec3[2] = val.z; }
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Vector 4 ---
        else if (importedoverride.type == "vec4") {
            asset->setOverride(id, Vector4(importedoverride.value_vec4[0], importedoverride.value_vec4[1], importedoverride.value_vec4[2], importedoverride.value_vec4[3]));

            auto field = new editor::InspectorField<Vector4>();
            field->name = id;
            field->getter = [asset, id]() { return asset->Get_load_data().overrides.at(id).value_vec4; };
            field->setter = [asset, id](Vector4 val) {
                asset->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) { i.value_vec4[0] = val.x; i.value_vec4[1] = val.y; i.value_vec4[2] = val.z; i.value_vec4[3] = val.w; }
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Texture ---
        else if (importedoverride.type == "texture") {
            asset->setTextureOverride(id, asset::Texture::GetAssetById(importedoverride.value_tex), importedoverride.tex_stage);

            auto field = new editor::InspectorTexture();
            field->name = id;
            field->getter = [asset, id]() {
                auto tex = asset->Get_load_data().overrides.at(id).value_tex;
                return tex ? tex->Get_assetId() : "";
                };
            field->setter = [asset, id](std::string val) {
                int stage = asset->Get_load_data().overrides.at(id).tex_stage;
                asset->setTextureOverride(id, asset::Texture::GetAssetById(val), stage);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) i.value_tex = val;
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
    }


    asset->SetInspectorData(newinspectordata);

    return MaterialData{
        //NONE
    };
}

void MaterialLoader::UnLoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, asset::data::MaterialLoadData* data) {

}

