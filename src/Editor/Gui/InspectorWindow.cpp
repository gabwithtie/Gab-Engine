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
		std::string cur_label = "";
		cur_label += typeid(*(*this->selected)[0]).name();

		ImGui::Text(cur_label.c_str());

		//FOR ONLY ONE
		auto inspectordata = (*this->selected)[0]->GetInspectorData();

		//DRAW THE BUILT IN INSPECTORS PER OBJECT

		ImGui::SeparatorText("Transform:");

		Vector3 position_gui_wrap = (*this->selected)[0]->Local().position.Get();
		this->DrawVector3Field("Position:", & position_gui_wrap);
		(*this->selected)[0]->Local().position.Set(position_gui_wrap);

		Vector3 scale_gui_wrap = (*this->selected)[0]->Local().scale.Get();
		this->DrawVector3Field("Scale:", &scale_gui_wrap);
		(*this->selected)[0]->Local().scale.Set(scale_gui_wrap);

		//DRAW THE CUSTOM INSPECTORS
		for (auto& field : inspectordata->fields)
		{
			if (field->fieldtype == editor::InspectorField::VECTOR3) {
				auto vec3field = static_cast<editor::InspectorVec3*>(field);
				Vector3 proxy_vec = {*vec3field->x, *vec3field->y, *vec3field->z};

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

void gbe::editor::InspectorWindow::DrawVector3Field(std::string label, Vector3* field)
{

	ImGui::Text(label.c_str()); // Optional: Display a label for the entire vector field
	ImGui::SameLine();

	auto remaining_width = ImGui::GetContentRegionAvail().x;
	auto per_width = remaining_width * (1.0f / 3.0f);

	ImGui::PushID(label.c_str());

	// Input for X component
	ImGui::SetNextItemWidth(per_width); // Adjust width as needed
	ImGui::InputFloat("##X", &field->x, 0.0f, 0.0f, "%.3f");
	ImGui::SameLine();

	// Input for Y component
	ImGui::SetNextItemWidth(per_width);
	ImGui::InputFloat("##Y", &field->y, 0.0f, 0.0f, "%.3f");
	ImGui::SameLine();

	// Input for Z component
	ImGui::SetNextItemWidth(per_width);
	ImGui::InputFloat("##Z", &field->z, 0.0f, 0.0f, "%.3f");
	
	ImGui::PopID();
}

std::string gbe::editor::InspectorWindow::GetWindowId()
{
	return "Inspector";
}
