#include "LightExplorer.h"

#include <type_traits>

#include "../Editor.h"
#include "../Utility/CreateFunctions.h"

void gbe::editor::LightExplorer::DrawSelf()
{
	if (ImGui::BeginTable("LightTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("Color");

		ImGui::TableHeadersRow();

		if (Engine::GetCurrentRoot() != nullptr)
			this->DrawChildList(Engine::GetCurrentRoot(), "");

		ImGui::EndTable();
	}

	Object* created_object = nullptr;

	if (ImGui::BeginPopupContextWindow("HierarchyContextMenu"))
	{
		if (ImGui::BeginMenu("Create"))
		{
			for (const auto& item : CreateFunctions::createfunctions_light)
			{
				if (ImGui::MenuItem(item.first.c_str())) {
					created_object = item.second();
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}

	if (created_object != nullptr) {
		created_object->SetParent(Engine::GetCurrentRoot());
	}
}

std::string gbe::editor::LightExplorer::GetWindowId()
{
	return "LightExplorer";
}

void gbe::editor::LightExplorer::DrawChildList(Object* parent, std::string label, unsigned int id)
{
	LightObject* lightobject = nullptr;
	lightobject = dynamic_cast<LightObject*>(parent);

	if (lightobject != nullptr) {
		ImGui::PushID(id);
		ImGui::TableNextRow(); // Start a new row

		ImGui::TableNextColumn(); // Move to the first column
		if (ImGui::Selectable(label.c_str())) {
			gbe::Editor::SelectSingle(parent);
		}

		ImGui::TableNextColumn();
		switch (lightobject->GetData()->type)
		{
		case gfx::Light::DIRECTIONAL:
			ImGui::Text("DIRECTIONAL");
			break;
		case gfx::Light::CONE:
			ImGui::Text("CONE");
			break;
		}

		ImGui::TableNextColumn();
		std::string field_id = "##color";
		ImGui::ColorEdit3(field_id.c_str(), &lightobject->GetData()->color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);


		ImGui::PopID();
	}

	for (size_t i = 0; i < parent->GetChildCount(); i++)
	{
		id++;

		auto child = parent->GetChildAt(i);

		std::string button_label = "";
		button_label += child->GetName();

		if (!child->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
			this->DrawChildList(child, button_label, id);
	}
}