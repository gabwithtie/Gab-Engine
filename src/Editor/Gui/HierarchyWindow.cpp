#include "HierarchyWindow.h"

#include "../Editor.h"

void gbe::editor::HierarchyWindow::DrawSelf()
{
	if (Engine::GetCurrentRoot() != nullptr)
		this->DrawChildList(Engine::GetCurrentRoot(), "");
}

std::string gbe::editor::HierarchyWindow::GetWindowId()
{
	return "Hierarchy";
}

void gbe::editor::HierarchyWindow::DrawChildList(Object* parent, std::string label, unsigned int id)
{
	bool expanded = false;

	if (label.size() > 0) {
		ImGui::PushID(id);

		expanded = ImGui::TreeNode("|");
		ImGui::SameLine();
		if (ImGui::Button(label.c_str())) {
			gbe::Editor::SelectSingle(parent);
		}
	}

	if (expanded || label.size() == 0)
	{
		for (size_t i = 0; i < parent->GetChildCount(); i++)
		{
			id++;

			auto child = parent->GetChildAt(i);

			std::string button_label = "";
			button_label += "[";
			button_label += std::to_string(i);
			button_label += "] : ";
			button_label += typeid(*child).name();

			if (!child->Get_is_editor())
				this->DrawChildList(child, button_label, id);
		}

		if (label.size() > 0) // Only pop if parent was drawn
			ImGui::TreePop();
	}
	if (label.size() > 0)
		ImGui::PopID();
}
