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
					auto f = static_cast<editor::InspectorBool*>(field);
					bool proxy_f = *f->x;

					ImGui::PushID(f->name.c_str());

					DrawFieldLabel(f->name);
					std::string field_id = "##" + f->name;

					if (ImGui::Checkbox(field_id.c_str(), &proxy_f))
					{
						*f->x = proxy_f;
						if (f->onchange)
							f->onchange();
					}

					ImGui::PopID();
				}

				if (field->fieldtype == editor::InspectorField::FLOAT) {
					auto f = static_cast<editor::InspectorFloat*>(field);
					float proxy_f = *f->x;

					ImGui::PushID(f->name.c_str());

					DrawFieldLabel(f->name);
					std::string field_id = "##" + f->name;

					if (ImGui::InputFloat(field_id.c_str(), &proxy_f, 0, 0, "%.6f"))
					{
						*f->x = proxy_f;

						if (f->onchange)
							f->onchange();
					}

					ImGui::PopID();
				}

				if (field->fieldtype == editor::InspectorField::VECTOR3) {
					auto f = static_cast<editor::InspectorColor*>(field);
					Vector3 proxy_f = { *f->r, *f->g, *f->b };

					if (this->DrawVector3Field(f->name.c_str(), &proxy_f)) {
						if (f->onchange)
							f->onchange();
					}

					*f->r = proxy_f.x;
					*f->g = proxy_f.y;
					*f->b = proxy_f.z;
				}

				if (field->fieldtype == editor::InspectorField::COLOR) {
					auto f = static_cast<editor::InspectorVec3*>(field);
					Vector3 proxy_f = { *f->x, *f->y, *f->z };

					DrawFieldLabel(f->name);
					std::string field_id = "##" + f->name;
					if (ImGui::ColorEdit3(field_id.c_str(), &proxy_f.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
						if (f->onchange)
							f->onchange();
					}

					*f->x = proxy_f.x;
					*f->y = proxy_f.y;
					*f->z = proxy_f.z;
				}

				if (field->fieldtype == editor::InspectorField::FUNCTION) {
					auto buttonfield = static_cast<editor::InspectorButton*>(field);
					if (ImGui::Button(buttonfield->name.c_str())) {
						buttonfield->onpress();
					}
				}

				if (field->fieldtype == editor::InspectorField::CHOICE) {
					auto f = static_cast<editor::InspectorChoice*>(field);
					int proxy_f = *f->index;

					if (ImGui::BeginCombo(f->name.c_str(), (*f->labels)[proxy_f].c_str()))
					{
						for (size_t n = 0; n < f->labels->size(); n++)
						{
							auto& cur = (*f->labels)[n];

							const bool is_selected = proxy_f == n;
							if (ImGui::Selectable(cur.c_str(), is_selected))
							{
								*f->index = n;

								f->onchange();
							}

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
