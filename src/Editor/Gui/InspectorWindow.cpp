#include "InspectorWindow.h"

#include "../Utility/ModelExport.h"
#include "../Editor.h"

#include "Asset/gbe_asset.h"

void gbe::editor::InspectorWindow::DrawSelf() {
	//OBJECT INSPECTOR
	if (this->selected.size() == 0) {
		if (this->is_reparenting) {
			ImGui::Text(("Select a new object at the hierarchy to reparent [" + this->reparentee->GetName() + "]").c_str());
		}
		else {
			ImGui::Text("Nothing Selected.");
		}
	}
	else if (this->selected.size() == 1)
	{
		if (is_reparenting)
		{
			auto reparentee_id = this->reparentee->Get_id();
			auto new_parent_id = this->selected[0]->Get_id();
			auto old_parent_id = this->reparentee->GetParent()->Get_id();

			Editor::CommitAction(
				[=]() {
					auto _reparentee = Engine::GetCurrentRoot()->GetObjectWithId(reparentee_id);
					auto _newparent = Engine::GetCurrentRoot()->GetObjectWithId(new_parent_id);

					_reparentee->SetParent(_newparent);
				},
				[=]() {
					auto _reparentee = Engine::GetCurrentRoot()->GetObjectWithId(reparentee_id);
					auto _oldparent = Engine::GetCurrentRoot()->GetObjectWithId(old_parent_id);

					_reparentee->SetParent(_oldparent);
				}
			);

			this->is_reparenting = false;

			Console::Log("Successfully reparented object.");
		}

		std::string cur_label = "";
		cur_label += this->selected[0]->GetName();

		ImGui::Text(cur_label.c_str());

		//FOR ONLY ONE
		auto inspectordata = this->selected[0]->GetInspectorData();

		//DRAW THE BUILT IN INSPECTORS PER OBJECT

		if (!this->selected[0]->GetEditorFlag(Object::IS_STATE_MANAGED)) {
			ImGui::SeparatorText("Quick Actions:");
			bool _enabled = this->selected[0]->Get_enabled_self();
			if (ImGui::Checkbox("Enabled", &_enabled)) {
				this->selected[0]->Set_enabled(_enabled);
			}
			ImGui::SameLine();
			if (ImGui::Button("Re-Parent")) {
				this->reparentee = this->selected[0];
				this->is_reparenting = true;
				Editor::DeselectAll();

				return;
			}
			ImGui::SameLine();
			if (ImGui::Button("Destroy")) {

				auto parent_id = this->selected[0]->GetParent()->Get_id();
				auto destroy_id = this->selected[0]->Get_id();
				auto respawn_info = this->selected[0]->Serialize();

				Editor::DeselectAll();

				Editor::CommitAction(
					[=]() {
						Engine::GetCurrentRoot()->GetObjectWithId(destroy_id)->Destroy();
					},
					[=]() {
						auto undoed = gbe::TypeSerializer::Instantiate(respawn_info.type, respawn_info);
						undoed->Deserialize(respawn_info);
						undoed->Set_id(destroy_id);
						undoed->SetParent(Engine::GetCurrentRoot()->GetObjectWithId(parent_id));
					}
				);

				return;
			}
		}

		if (ImGui::CollapsingHeader("Transform")) {

			auto changed_id = this->selected[0]->Get_id();

			Vector3 position_gui_wrap = this->selected[0]->Local().position.Get();
			Vector3 position_gui_wrap_old = position_gui_wrap;
			if (this->DrawVector3Field("Position:", &position_gui_wrap,
				!this->selected[0]->GetEditorFlag(Object::STATIC_POS_X),
				!this->selected[0]->GetEditorFlag(Object::STATIC_POS_Y),
				!this->selected[0]->GetEditorFlag(Object::STATIC_POS_Z)
			)) {
				Editor::CommitAction(
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
						modified->Local().position.Set(position_gui_wrap);
						selected[0]->PushState(Object::TRANSFORMED_USER);
					},
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
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
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
						modified->Local().scale.Set(scale_gui_wrap);
						selected[0]->PushState(Object::TRANSFORMED_USER);
					},
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
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
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
						modified->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap));
						selected[0]->PushState(Object::TRANSFORMED_USER);
					},
					[=]() {
						auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
						modified->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap_old));
						selected[0]->PushState(Object::TRANSFORMED_USER);
					}
				);
			}
		}

		if (inspectordata->fields.size() > 0) {
			ImGui::SeparatorText((cur_label + " Specific").c_str());

			//DRAW THE CUSTOM INSPECTORS
			for (auto& field : inspectordata->fields)
			{
				if (field->fieldtype == editor::InspectorField::VECTOR3) {
					auto vec3field = static_cast<editor::InspectorVec3*>(field);
					Vector3 proxy_vec = { *vec3field->x, *vec3field->y, *vec3field->z };

					this->DrawVector3Field(vec3field->name.c_str(), &proxy_vec);

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

	//EXPORTING
	if (this->selected.size() > 0) {
		ImGui::SeparatorText("Exporting");
		if (ImGui::Button("Merge and export selected.")) {
			ModelExport modelexporter(selected);

			modelexporter.Export("merged.obj");

			std::cout << "Wrote merged mesh file.";
		}
	}
}

bool gbe::editor::InspectorWindow::DrawVector3Field(std::string label, Vector3* field, bool x_interactable, bool y_interactable, bool z_interactable)
{
	ImGui::PushID(label.c_str());

	float vec_arr[3] = { field->x, field->y, field->z };

	std::string floatlabel = "##" + label;
	ImGui::Text(label.c_str());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1);
	bool changed = ImGui::DragFloat3(floatlabel.c_str(), vec_arr);

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

std::string gbe::editor::InspectorWindow::GetWindowId()
{
	return "Inspector";
}
