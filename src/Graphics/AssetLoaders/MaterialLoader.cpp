#include "MaterialLoader.h"
#include "Editor/gbe_editor.h"

#include "Editor/Utility/FileDialogue.h"

using namespace gbe;
using namespace gfx;

void MaterialLoader::LoadAsset_(asset::Material* asset, const asset::data::MaterialImportData& importdata, MaterialData* data) {
    data->shader = asset::Shader::GetAssetById(importdata.shader);
    data->shadowcaster = importdata.shadowcaster != 0;
    data->defaultrendergroup = importdata.defaultrendergroup;

    auto newinspectordata = new editor::InspectorData();

    for (const auto& importedoverride : importdata.overrides)
    {
        std::string id = importedoverride.id;

        // --- Float / Single ---
        if (importedoverride.type == "single") {
            data->setOverride(id, importedoverride.value_single);

            auto field = new editor::InspectorFloat();
            field->name = id;
            field->getter = [data, id]() { return data->overrides.at(id).value_float; };
            field->setter = [=](float val) {
                data->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) i.value_single = val;
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Vector 2 ---
        else if (importedoverride.type == "vec2") {
            data->setOverride(id, Vector2(importedoverride.value_vec2[0], importedoverride.value_vec2[1]));

            auto field = new editor::InspectorVec2();
            field->name = id;
            field->getter = [data, id]() { return data->overrides.at(id).value_vec2; };
            field->setter = [=](Vector2 val) {
                data->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) { i.value_vec2[0] = val.x; i.value_vec2[1] = val.y; }
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Vector 3 / Color ---
        else if (importedoverride.type == "vec3") {
            data->setOverride(id, Vector3(importedoverride.value_vec3[0], importedoverride.value_vec3[1], importedoverride.value_vec3[2]));

            editor::InspectorField<Vector3>* field;
            if (id.find("Color") != std::string::npos || id.find("color") != std::string::npos)
                field = new editor::InspectorColor();
            else
                field = new editor::InspectorVec3();

            field->name = id;
            field->getter = [data, id]() { return data->overrides.at(id).value_vec3; };
            field->setter = [=](Vector3 val) {
                data->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) { i.value_vec3[0] = val.x; i.value_vec3[1] = val.y; i.value_vec3[2] = val.z; }
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Vector 4 ---
        else if (importedoverride.type == "vec4") {
            data->setOverride(id, Vector4(importedoverride.value_vec4[0], importedoverride.value_vec4[1], importedoverride.value_vec4[2], importedoverride.value_vec4[3]));

            auto field = new editor::InspectorVec4();
            field->name = id;
            field->getter = [data, id]() { return data->overrides.at(id).value_vec4; };
            field->setter = [=](Vector4 val) {
                data->setOverride(id, val);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) { i.value_vec4[0] = val.x; i.value_vec4[1] = val.y; i.value_vec4[2] = val.z; i.value_vec4[3] = val.w; }
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
        // --- Texture ---
        else if (importedoverride.type == "texture") {
            data->setTextureOverride(id, asset::Texture::GetAssetById(importedoverride.value_tex), importedoverride.tex_stage);

            auto field = new editor::InspectorTexture();
            field->name = id;
            field->getter = [data, id]() {
                auto tex = data->overrides.at(id).value_tex;
                return tex ? tex->Get_assetId() : "";
                };
            field->setter = [=](std::string val) {
                int stage = data->overrides.at(id).tex_stage;
                data->setTextureOverride(id, asset::Texture::GetAssetById(val), stage);
                for (auto& i : asset->Get_import_data().overrides) if (i.id == id) i.value_tex = val;
                asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), asset->Get_asset_filepath());
                };
            newinspectordata->fields.push_back(field);
        }
    }

    {
        {
            // Define the button for duplication
            auto duplicateBtn = new editor::InspectorButton(); // Assuming standard inspector button class
            duplicateBtn->name = "Duplicate Material & Textures";
            duplicateBtn->onpress = [asset, data]() {
                namespace fs = std::filesystem;

                // 1. Get the target directory from the user
                std::string selectedDir = editor::FileDialogue::GetFilePath(editor::FileDialogue::FOLDER);
                if (selectedDir.empty()) return;

                fs::path targetDir(selectedDir);
                std::string folderName = targetDir.filename().string();

                // 2. Prepare Material duplication
                fs::path originalMatPath = asset->Get_asset_filepath();
                std::string newMatName = originalMatPath.stem().stem().string() + "_" + folderName + ".mat.gbe";
                // .stem().stem() handles "name.mat.gbe" -> "name"

                fs::path targetMatPath = targetDir / newMatName;

                // 3. Duplicate attached textures
                for (auto& [id, materialOverride] : data->overrides) {
                    if (materialOverride.type == asset::Shader::UniformFieldType::TEXTURE && materialOverride.value_tex) {
                        auto texture = materialOverride.value_tex;

						if (texture == nullptr) continue;

                        // Get paths for the actual image file
                        fs::path texMetaPath = texture->Get_asset_filepath();
                        auto& texImportData = texture->Get_import_data();

                        fs::path sourceImagePath = texMetaPath.parent_path() / texImportData.path;
                        std::string newImageName = sourceImagePath.stem().string() + "_" + folderName + sourceImagePath.extension().string();
                        fs::path targetImagePath = targetDir / newImageName;

                        try {
                            // Copy the raw image file
                            if (fs::exists(sourceImagePath)) {
                                fs::copy_file(sourceImagePath, targetImagePath, fs::copy_options::overwrite_existing);
                            }
                        }
                        catch (const fs::filesystem_error& e) {
                            printf("[MaterialLoader] Failed to copy texture: %s\n", e.what());
                        }
                    }
                }

                // 4. Duplicate the Material source file
                // Note: We copy the material logic/data, not the .gbe metafile directly, 
                // as BatchLoader will generate a new metafile for the duplicate.
                try {
                    // Since materials are often just the metafile or a specific source, 
                    // we ensure the core data is exported to the new location.
                    asset::serialization::gbeParser::ExportClass<asset::data::MaterialImportData>(asset->Get_import_data(), targetMatPath);

                    // 5. Initialize the new directory
					asset::BatchLoader::ReloadDirectory(targetDir);

                    printf("[MaterialLoader] Successfully duplicated material to: %s\n", targetDir.string().c_str());
                }
                catch (const std::exception& e) {
                    printf("[MaterialLoader] Duplication Error: %s\n", e.what());
                }
                };
            newinspectordata->fields.push_back(duplicateBtn);
        }
    }

    asset->SetInspectorData(newinspectordata);
}

void MaterialLoader::UnLoadAsset_(MaterialData* data) {

}

