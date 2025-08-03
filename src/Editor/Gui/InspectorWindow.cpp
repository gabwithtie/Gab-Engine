#include "InspectorWindow.h"

#include "../Utility/ModelExport.h"
#include "../Editor.h"

void gbe::editor::InspectorWindow::DrawSelf() {
	//OBJECT INSPECTOR
	if ((*this->selected).size() == 0) {
		ImGui::Text("Nothing Selected.");
	}
	else if ((*this->selected).size() == 1)
	{
		//FOR ONLY ONE
		auto inspectordata = (*this->selected)[0]->GetInspectorData();

		//DRAW THE BUILT IN INSPECTORS PER OBJECT
		auto sel_parent = (*this->selected)[0]->GetParent();
		if (sel_parent != nullptr) {
			std::string button_label = "Select Parent ";
			button_label += "[";
			button_label += typeid(*sel_parent).name();
			button_label += "]";

			if (ImGui::Button(button_label.c_str())) {
				gbe::Editor::SelectSingle(sel_parent);
			}
		}

		ImGui::SeparatorText("Transform:");

		Vector3 position_gui_wrap = (*this->selected)[0]->Local().position.Get();

		ImGui::InputFloat("pos.X", &position_gui_wrap.x);
		ImGui::InputFloat("pos.Y", &position_gui_wrap.y);
		ImGui::InputFloat("pos.Z", &position_gui_wrap.z);

		(*this->selected)[0]->Local().position.Set(position_gui_wrap);

		Vector3 scale_gui_wrap = (*this->selected)[0]->Local().scale.Get();

		ImGui::InputFloat("scale.X", &scale_gui_wrap.x);
		ImGui::InputFloat("scale.Y", &scale_gui_wrap.y);
		ImGui::InputFloat("scale.Z", &scale_gui_wrap.z);

		(*this->selected)[0]->Local().scale.Set(scale_gui_wrap);

		//DRAW THE CUSTOM INSPECTORS
		for (auto& field : inspectordata->fields)
		{
			if (field->fieldtype == editor::InspectorField::VECTOR3) {
				auto vec3field = static_cast<editor::InspectorVec3*>(field);

				ImGui::SeparatorText(vec3field->name.c_str());

				ImGui::InputFloat("X", vec3field->x);
				ImGui::InputFloat("Y", vec3field->y);
				ImGui::InputFloat("Z", vec3field->z);
			}

			if (field->fieldtype == editor::InspectorField::FUNCTION) {
				auto buttonfield = static_cast<editor::InspectorButton*>(field);
				if (ImGui::Button(buttonfield->name.c_str())) {
					buttonfield->onpress();
				}
			}
		}

		//DRAW THE CHILDREN SELECTOR
		ImGui::SeparatorText("CHILDREN");
		for (size_t i = 0; i < (*this->selected)[0]->GetChildCount(); i++)
		{
			auto child = (*this->selected)[0]->GetChildAt(i);

			std::string button_label = "";
			button_label += "[";
			button_label += std::to_string(i);
			button_label += "] : ";
			button_label += typeid(*child).name();

			if (ImGui::Button(button_label.c_str())) {
				gbe::Editor::SelectSingle(child);
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

std::string gbe::editor::InspectorWindow::GetWindowId()
{
	return "Inspector";
}
