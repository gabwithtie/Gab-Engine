#include "InspectorWindow.h"

#include "../../Utility/ModelExport.h"
#include "../../Editor.h"

#include "Asset/gbe_asset.h"

void gbe::editor::InspectorWindow::DrawSelf() {
	//FOR ONLY ONE
	if (this->selected.size() == 1)
	{
		auto selected_id = this->selected[0]->Get_id();
		auto inspectordata = this->selected[0]->GetInspectorData();

		//DRAW THE BUILT IN INSPECTORS PER OBJECT

		if (ImGui::CollapsingHeader("Info")) {
			//NAME
			ImGui::Text("Name:");
			ImGui::SameLine();

			static char nameBuffer[128];
			std::string name = this->selected[0]->GetName();
			strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
			nameBuffer[sizeof(nameBuffer) - 1] = '\0'; // Ensure null termination

			static bool isEditingName = false;

			if (isEditingName) {
				// 1. Force focus so the user can start typing immediately
				ImGui::SetKeyboardFocusHere();

				// 2. Use EnterReturnsTrue to confirm the change
				if (ImGui::InputText("##editname", nameBuffer, IM_ARRAYSIZE(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
					isEditingName = false;
					this->selected[0]->SetName(nameBuffer);
				}

				// 3. De-focusing (clicking away) also saves/closes
				if (ImGui::IsItemDeactivated()) {
					isEditingName = false;
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
			
			//Enabled
			if (!this->selected[0]->GetEditorFlag(Object::NON_DIRECT_EDITABLE)) {
				ImGui::Text("Enabled:");
				ImGui::SameLine();

				bool _enabled = this->selected[0]->Get_enabled_self();
				if (ImGui::Checkbox("##enabled", &_enabled)) {
					this->selected[0]->Set_enabled(_enabled);
				}
			}
		}

		if (ImGui::CollapsingHeader("Transform")) {
			Vector3 position_gui_wrap = this->selected[0]->Local().position.Get();
			Vector3 position_gui_wrap_old = position_gui_wrap;
			if (this->DrawVector3Field("Position:", &position_gui_wrap,
				!this->selected[0]->GetEditorFlag(Object::STATIC_POS_X),
				!this->selected[0]->GetEditorFlag(Object::STATIC_POS_Y),
				!this->selected[0]->GetEditorFlag(Object::STATIC_POS_Z)
			)) {
				Editor::CommitAction(
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(selected_id);
						modified->Local().position.Set(position_gui_wrap);
						selected[0]->PushState(Object::TRANSFORMED_USER);
					},
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(selected_id);
						modified->Local().position.Set(position_gui_wrap_old);
						selected[0]->PushState(Object::TRANSFORMED_USER);
					}
				);
			}

			Vector3 scale_gui_wrap = this->selected[0]->Local().scale.Get();
			Vector3 scale_gui_wrap_old = scale_gui_wrap;
			if (this->DrawVector3Field("Scale:", &scale_gui_wrap,
				!this->selected[0]->GetEditorFlag(Object::STATIC_SCALE_X),
				!this->selected[0]->GetEditorFlag(Object::STATIC_SCALE_Y),
				!this->selected[0]->GetEditorFlag(Object::STATIC_SCALE_Z)
			)) {
				Editor::CommitAction(
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(selected_id);
						modified->Local().scale.Set(scale_gui_wrap);
						selected[0]->PushState(Object::TRANSFORMED_USER);
					},
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(selected_id);
						modified->Local().scale.Set(scale_gui_wrap_old);
						selected[0]->PushState(Object::TRANSFORMED_USER);
					}
				);
			}

			Vector3 rot_gui_wrap = this->selected[0]->Local().rotation.Get().ToEuler();
			Vector3 rot_gui_wrap_old = rot_gui_wrap;
			if (this->DrawVector3Field("Rotation:", &rot_gui_wrap
				, !this->selected[0]->GetEditorFlag(Object::STATIC_ROT_X)
				, !this->selected[0]->GetEditorFlag(Object::STATIC_ROT_Y)
				, !this->selected[0]->GetEditorFlag(Object::STATIC_ROT_Z)
			)) {
				Editor::CommitAction(
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(selected_id);
						modified->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap));
						selected[0]->PushState(Object::TRANSFORMED_USER);
					},
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(selected_id);
						modified->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap_old));
						selected[0]->PushState(Object::TRANSFORMED_USER);
					}
				);
			}
		}

		if (inspectordata->fields.size() > 0) {
			//DRAW THE CUSTOM INSPECTORS
			for (auto& field : inspectordata->fields)
			{
				if (field->fieldtype == editor::InspectorField::BOOLEAN) {
					auto boolfield = static_cast<editor::InspectorBool*>(field);
					bool proxy_bool = *boolfield->x;

					ImGui::PushID(boolfield->name.c_str());

					DrawFieldLabel(boolfield->name);
					std::string field_id = "##" + boolfield->name;
					bool changed = ImGui::Checkbox(field_id.c_str(), &proxy_bool);

					if (changed)
					{
						*boolfield->x = proxy_bool;
					}

					ImGui::PopID();
				}

				if (field->fieldtype == editor::InspectorField::FLOAT) {
					auto floatfield = static_cast<editor::InspectorFloat*>(field);
					float proxy_float = *floatfield->x;

					ImGui::PushID(floatfield->name.c_str());

					DrawFieldLabel(floatfield->name);
					std::string field_id = "##" + floatfield->name;
					bool changed = ImGui::InputFloat(field_id.c_str(), &proxy_float, 0, 0, "%.6f");

					if (changed)
					{
						*floatfield->x = proxy_float;

						if (floatfield->onchange) {
							floatfield->onchange();
						}
					}

					ImGui::PopID();
				}

				if (field->fieldtype == editor::InspectorField::VECTOR3) {
					auto vec3field = static_cast<editor::InspectorColor*>(field);
					Vector3 proxy_vec = { *vec3field->r, *vec3field->g, *vec3field->b };

					this->DrawVector3Field(vec3field->name.c_str(), &proxy_vec);

					*vec3field->r = proxy_vec.x;
					*vec3field->g = proxy_vec.y;
					*vec3field->b = proxy_vec.z;
				}

				if (field->fieldtype == editor::InspectorField::COLOR) {
					auto vec3field = static_cast<editor::InspectorVec3*>(field);
					Vector3 proxy_vec = { *vec3field->x, *vec3field->y, *vec3field->z };

					DrawFieldLabel(vec3field->name);
					std::string field_id = "##" + vec3field->name;
					ImGui::ColorEdit3(field_id.c_str(), &proxy_vec.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

					*vec3field->x = proxy_vec.x;
					*vec3field->y = proxy_vec.y;
					*vec3field->z = proxy_vec.z;
				}

				if (field->fieldtype == editor::InspectorField::FUNCTION) {
					auto buttonfield = static_cast<editor::InspectorButton*>(field);
					if (ImGui::Button(buttonfield->name.c_str())) {
						buttonfield->onpress();
					}
				}

				if (field->fieldtype == editor::InspectorField::ASSET) {
					auto assetfield = static_cast<editor::InspectorAsset_base*>(field);
					auto assetlist = assetfield->GetChoices();

					if (ImGui::BeginCombo(assetfield->name.c_str(), assetfield->choice_label.c_str()))
					{
						for (size_t n = 0; n < assetlist.size(); n++)
						{
							auto cur = assetlist[n]->Get_asset_filepath();

							const bool is_selected = (cur == assetfield->choice_label.c_str());
							if (ImGui::Selectable(cur.string().c_str(), is_selected))
							{
								*assetfield->choice = assetlist[n];
								assetfield->choice_label = cur.string();
							}

							// Set the initial focus when opening the combo (scrolling to the item if needed)
							if (is_selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
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
