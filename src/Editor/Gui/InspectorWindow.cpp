#include "InspectorWindow.h"

#include "../Utility/ModelExport.h"
#include "../Editor.h"

void gbe::editor::InspectorWindow::DrawSelf() {
	//OBJECT INSPECTOR
	if ((*this->selected).size() == 0) {
		if (this->is_reparenting) {
			std::string cur_label = typeid(*this->reparentee).name();
			ImGui::Text(("Select a new object at the hierarchy to reparent [" + cur_label + "]").c_str());
		}
		else {
			ImGui::Text("Nothing Selected.");
		}
	}
	else if ((*this->selected).size() == 1)
	{
		if (is_reparenting)
		{
			auto reparentee_id = this->reparentee->Get_id();
			auto new_parent_id = (*this->selected)[0]->Get_id();
			auto old_parent_id = this->reparentee->GetParent()->Get_id();

			Editor::RegisterAction(
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

			this->reparentee->SetParent((*this->selected)[0]);
			this->reparentee = nullptr;
			this->is_reparenting = false;

			Console::Log("Successfully reparented object.");
		}

		std::string cur_label = "";
		cur_label += typeid(*(*this->selected)[0]).name();

		ImGui::Text(cur_label.c_str());

		//FOR ONLY ONE
		auto inspectordata = (*this->selected)[0]->GetInspectorData();

		//DRAW THE BUILT IN INSPECTORS PER OBJECT

		ImGui::SeparatorText("Quick Actions:");
		bool _enabled = (*this->selected)[0]->Get_enabled_self();
		if (ImGui::Checkbox("Enabled", &_enabled)) {
			(*this->selected)[0]->Set_enabled(_enabled);
		}
		ImGui::SameLine();
		if (ImGui::Button("Re-Parent")) {
			this->reparentee = (*this->selected)[0];
			this->is_reparenting = true;
			Editor::DeselectAll();

			return;
		}
		ImGui::SameLine();
		if (ImGui::Button("Destroy")) {

			auto parent_id = (*this->selected)[0]->GetParent()->Get_id();
			auto destroy_id = (*this->selected)[0]->Get_id();
			auto respawn_info = (*this->selected)[0]->Serialize();

			(*this->selected)[0]->Destroy();
			
			Editor::DeselectAll();

			Editor::RegisterAction(
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

		ImGui::SeparatorText("Transform:");

		auto changed_id = (*this->selected)[0]->Get_id();

		Vector3 position_gui_wrap = (*this->selected)[0]->Local().position.Get();
		Vector3 position_gui_wrap_old = position_gui_wrap;
		if (this->DrawVector3Field("Position:", &position_gui_wrap)) {
			(*this->selected)[0]->Local().position.Set(position_gui_wrap);

			Editor::RegisterAction(
				[=]() {
					auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
					modified->Local().position.Set(position_gui_wrap);
				},
				[=]() {
					auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
					modified->Local().position.Set(position_gui_wrap_old);
				}
			);
		}

		Vector3 scale_gui_wrap = (*this->selected)[0]->Local().scale.Get();
		Vector3 scale_gui_wrap_old = scale_gui_wrap;
		if (this->DrawVector3Field("Scale:", &scale_gui_wrap)) {
			(*this->selected)[0]->Local().scale.Set(scale_gui_wrap);

			Editor::RegisterAction(
				[=]() {
					auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
					modified->Local().scale.Set(scale_gui_wrap);
				},
				[=]() {
					auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
					modified->Local().scale.Set(scale_gui_wrap_old);
				}
			);
		}

		Vector3 rot_gui_wrap = (*this->selected)[0]->Local().rotation.Get().ToEuler();
		Vector3 rot_gui_wrap_old = rot_gui_wrap;
		if (this->DrawVector3Field("Rotation:", &rot_gui_wrap)) {
			(*this->selected)[0]->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap));

			Editor::RegisterAction(
				[=]() {
					auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
					modified->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap));
				},
				[=]() {
					auto modified = Engine::GetCurrentRoot()->GetObjectWithId(changed_id);
					modified->Local().rotation.Set(Quaternion::Euler(rot_gui_wrap_old));
				}
			);
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
			}
		}
	}
	else {
		ImGui::Text("Multi-inspect not supported yet.");
	}

	//EXPORTING
	if ((*this->selected).size() > 0) {
		ImGui::SeparatorText("Exporting");
		if (ImGui::Button("Merge and export selected.")) {
			ModelExport modelexporter(*selected);

			modelexporter.Export("merged.obj");

			std::cout << "Wrote merged mesh file.";
		}
	}
}

bool gbe::editor::InspectorWindow::DrawVector3Field(std::string label, Vector3* field)
{

	ImGui::Text(label.c_str()); // Optional: Display a label for the entire vector field
	ImGui::SameLine();

	auto remaining_width = ImGui::GetContentRegionAvail().x;
	auto per_width = remaining_width * (1.0f / 3.0f);

	ImGui::PushID(label.c_str());

	// Input for X component
	ImGui::SetNextItemWidth(per_width); // Adjust width as needed
	bool changed_x = ImGui::InputFloat("##X", &field->x, 0.0f, 0.0f, "%.3f");
	ImGui::SameLine();

	// Input for Y component
	ImGui::SetNextItemWidth(per_width);
	bool changed_y = ImGui::InputFloat("##Y", &field->y, 0.0f, 0.0f, "%.3f");
	ImGui::SameLine();

	// Input for Z component
	ImGui::SetNextItemWidth(per_width);
	bool changed_z = ImGui::InputFloat("##Z", &field->z, 0.0f, 0.0f, "%.3f");
	
	ImGui::PopID();
	
	return changed_x || changed_y || changed_z;
}

std::string gbe::editor::InspectorWindow::GetWindowId()
{
	return "Inspector";
}
