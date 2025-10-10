#include "HierarchyWindow.h"

#include "../Editor.h"
#include "../Utility/CreateFunctions.h"
#include "LightExplorer.h"

void gbe::editor::HierarchyWindow::DrawSelf()
{
	if (Engine::GetCurrentRoot() != nullptr)
		this->DrawChildList(Engine::GetCurrentRoot(), "");

	Object* created_object = nullptr;

	if (created_object != nullptr) {
		created_object->SetParent(Engine::GetCurrentRoot());
	}
}

std::string gbe::editor::HierarchyWindow::GetWindowId()
{
	return "Hierarchy";
}

void gbe::editor::HierarchyWindow::DrawChildList(Object* parent, std::string label, unsigned int id)
{
	bool expanded = false;

	bool is_leaf = true;

	for (size_t i = 0; i < parent->GetChildCount(); i++)
	{
		auto child = parent->GetChildAt(i);
		if (!child->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
			is_leaf = false;
	}

	if (label.size() > 0) {
		ImGui::PushID(id);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow; // Or other desired flags

		if (is_leaf)
			flags |= ImGuiTreeNodeFlags_Leaf;

		expanded = ImGui::TreeNodeEx(label.c_str(), flags);

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
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
			button_label += child->GetName();

			if (!child->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
				this->DrawChildList(child, button_label, id);
		}

		if (label.size() > 0) // Only pop if parent was drawn
			ImGui::TreePop();
	}
	if (label.size() > 0)
		ImGui::PopID();
}
