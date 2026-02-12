#include "InspectorWindow.h"

#include "../../Utility/ModelExport.h"
#include "../../Editor.h"

#include "../Utility/AssetPickerPopup.h"

#include "../Utility/DragDrop.h"

#include "Asset/gbe_asset.h"

void gbe::editor::InspectorWindow::SetInspectorData(std::vector<InspectorData*> _data)
{
	this->data = _data;
}

void gbe::editor::InspectorWindow::DrawSelf() {
	//FOR ONLY ONE
	if (this->data.size() == 1)
	{
		auto& first_data = this->data[0];

		for (auto& field : first_data->fields)
		{
			if (field->fieldtype == editor::STRING) {
				auto f = static_cast<editor::InspectorString*>(field);
				ImGui::PushID(f->name.c_str());
				DrawFieldLabel(f->name);

				static char nameBuffer[128];
				std::string name = f->getter();
				strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
				nameBuffer[sizeof(nameBuffer) - 1] = '\0'; // Ensure null termination

				static bool isEditingName = false;

				if (isEditingName) {
					// 1. Force focus so the user can start typing immediately
					ImGui::SetKeyboardFocusHere();

					// 2. Use EnterReturnsTrue to confirm the change
					if (ImGui::InputText("##editname", nameBuffer, IM_ARRAYSIZE(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
						isEditingName = false;
						f->setter(nameBuffer);
					}

					// 3. De-focusing (clicking away) also saves/closes
					if (ImGui::IsItemDeactivated()) {
						isEditingName = false;
						f->setter(nameBuffer);
					}
				}
				else {
					// Render as a label or selectable
					if (ImGui::Selectable(nameBuffer)) {
						// Optional: Single click to select, logic handled elsewhere
					}

					// Double-click to trigger the rename state
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
						isEditingName = true;
					}
				}

				ImGui::PopID();
			}

			if (field->fieldtype == editor::BOOLEAN) {
				auto f = static_cast<editor::InspectorBool*>(field);
				bool proxy_f = f->getter();

				ImGui::PushID(f->name.c_str());

				DrawFieldLabel(f->name);
				std::string field_id = "##" + f->name;

				if (ImGui::Checkbox(field_id.c_str(), &proxy_f))
				{
					f->setter(proxy_f);
				}

				ImGui::PopID();
			}

			if (field->fieldtype == editor::FLOAT) {
				auto f = static_cast<editor::InspectorFloat*>(field);
				float proxy_f = f->getter();

				ImGui::PushID(f->name.c_str());

				DrawFieldLabel(f->name);
				std::string field_id = "##" + f->name;

				if (ImGui::InputFloat(field_id.c_str(), &proxy_f, 0, 0, "%.6f"))
				{
					f->setter(proxy_f);
				}

				ImGui::PopID();
			}

			if (field->fieldtype == editor::ASSET) {
				auto f = static_cast<editor::InspectorAsset*>(field);
				std::string proxy_f = f->getter();
				
				DraggableTarget(asset::MATERIAL,
					[=](const DragData& data) { f->setter(data.id); },
					[=]() {
						// 1. Draw the row label (the key/field name)
						DrawFieldLabel(f->name);

						// 2. Resolve current asset ID for the button label
						auto currentAsset = f->getter();
						std::string displayId = currentAsset.size() > 0 ? currentAsset : "None";
						std::string popupId = "Popup_" + f->name;

						// 3. Render the Picker Button
						// DrawFieldLabel already handled the SameLine offset and width
						if (ImGui::Button((displayId + "##btn").c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
							ImGui::OpenPopup(popupId.c_str());
						}

						if (AssetPickerPopup(
							f->name.c_str(),
							[=]() {
								return asset::all_asset_loaders[f->assettype]->FindAssetById(f->getter());
							}, f->setter, asset::all_asset_loaders[f->assettype]->GetAllAssetIds()
								)) {

						}
					}
					);
			}

			if (field->fieldtype == editor::VECTOR3) {
				auto f = static_cast<editor::InspectorVec3*>(field);
				Vector3 proxy_f = f->getter();

				if (this->DrawVector3Field(f->name.c_str(), &proxy_f)) {
					f->setter(proxy_f);
				}
			}

			if (field->fieldtype == editor::COLOR) {
				auto f = static_cast<editor::InspectorColor*>(field);
				Vector3 proxy_f = f->getter();

				DrawFieldLabel(f->name);
				std::string field_id = "##" + f->name;
				if (ImGui::ColorEdit3(field_id.c_str(), &proxy_f.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
					f->setter(proxy_f);
				}
			}

			if (field->fieldtype == editor::TEXTURE) {
				auto f = static_cast<editor::InspectorTexture*>(field);
				auto proxy_f = f->getter();

				TextureAssetPicker(
					f->name.c_str(),
					ImVec2(64, 64),
					[=]() {
						auto asset = gbe::asset::Texture::GetAssetById(proxy_f);
						return asset;
					},
					[=](std::string new_asset_id) {
						f->setter(new_asset_id);
					},
					asset::all_asset_loaders[asset::TEXTURE]->GetAllAssetIds()
				);
			}

			if (field->fieldtype == editor::FUNCTION) {
				auto buttonfield = static_cast<editor::InspectorButton*>(field);
				if (ImGui::Button(buttonfield->name.c_str())) {
					buttonfield->onpress();
				}
			}

			if (field->fieldtype == editor::CHOICE) {
				auto f = static_cast<editor::InspectorChoice*>(field);
				int proxy_f = f->getter();

				if (ImGui::BeginCombo(f->name.c_str(), (*f->labels)[proxy_f].c_str()))
				{
					for (size_t n = 0; n < f->labels->size(); n++)
					{
						auto& cur = (*f->labels)[n];

						const bool is_selected = proxy_f == n;
						if (ImGui::Selectable(cur.c_str(), is_selected))
						{
							f->setter(n);
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}

			if (field->fieldtype == editor::DICTIONARY) {
				auto f = static_cast<editor::InspectorAssetDictionary*>(field);

				// Create a collapsible group for the dictionary
				if (ImGui::TreeNodeEx(f->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					for (const auto& key : *f->fieldList) {
						ImGui::PushID(key.c_str());

						// 1. Draw the row label (the key/field name)
						DrawFieldLabel(key);

						// 2. Resolve current asset ID for the button label
						auto currentAsset = f->a_getter(key);
						std::string displayId = currentAsset ? currentAsset->Get_assetId() : "None";
						std::string popupId = "Popup_" + key;

						// 3. Render the Picker Button
						// DrawFieldLabel already handled the SameLine offset and width
						if (ImGui::Button((displayId + "##btn").c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
							ImGui::OpenPopup(popupId.c_str());
						}

						auto poollist = asset::all_asset_loaders[f->assettype]->GetAllAssetIds();

						// 4. Implement the Popup Logic
						AssetPickerPopup(
							popupId.c_str(),
							[f, key]() { return f->a_getter(key); }, // Getter for current asset
							[f, key](std::string selectedId) {
								// Iterate all loaders to find the pointer corresponding to the selected ID
								asset::internal::BaseAsset_base* foundAsset = asset::all_asset_loaders[f->assettype]->FindAssetById(selectedId);
								// Update the dictionary via the provided setter
								f->a_setter(key, foundAsset);
							},
							poollist // Pool of available assets to pick from
						);

						ImGui::PopID();
					}
					ImGui::TreePop();
				}
			}
		}
	}
	else {
		ImGui::Text("Multi-inspect not supported yet.");
	}
}

bool gbe::editor::InspectorWindow::DrawVector3Field(std::string label, Vector3* field, bool x_interactable, bool y_interactable, bool z_interactable)
{
	ImGui::PushID(label.c_str());

	float vec_arr[3] = { field->x, field->y, field->z };

	DrawFieldLabel(label);

	std::string field_id = "##" + label;
	bool changed = ImGui::DragFloat3(field_id.c_str(), vec_arr);

	if (changed)
	{
		if (x_interactable)
			field->x = vec_arr[0];
		if (y_interactable)
			field->y = vec_arr[1];
		if (z_interactable)
			field->z = vec_arr[2];
	}

	ImGui::PopID();
	
	return changed;
}

void gbe::editor::InspectorWindow::DrawFieldLabel(std::string label) {
	ImGui::Text(label.c_str());
	float windowWidth = ImGui::GetContentRegionAvail().x;
	float xOffset = (windowWidth) * 0.4f;
	ImGui::SameLine(xOffset);
	ImGui::SetNextItemWidth(-1);
}

std::string gbe::editor::InspectorWindow::GetWindowId()
{
	return "Inspector";
}
