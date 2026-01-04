#include "AnitoBuilderWindow.h"

#include "Ext/AnitoBuilderWrapper/BuilderBlock.h"
#include "../Editor.h"
#include "../Utility/CreateFunctions.h"

void gbe::editor::AnitoBuilderWindow::DrawSelf()
{
	if (ImGui::Button("Toggle Model")) {
		ext::AnitoBuilder::BuilderBlock::ToggleModel();
	}

	ImGui::SeparatorText("Block Explorer");
	if (ImGui::BeginTable("BlockTable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
	{
		ImGui::TableSetupColumn("Name");

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
			for (const auto& item : CreateFunctions::createfunctions_ext_anitobuilder)
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

std::string gbe::editor::AnitoBuilderWindow::GetWindowId()
{
	return "Anito Builder Explorer";
}

void gbe::editor::AnitoBuilderWindow::DrawChildList(Object* parent, std::string label, unsigned int id)
{
	ext::AnitoBuilder::BuilderBlock* blockobject = nullptr;
	blockobject = dynamic_cast<ext::AnitoBuilder::BuilderBlock*>(parent);

	if (blockobject != nullptr) {
		ImGui::PushID(id);
		ImGui::TableNextRow(); // Start a new row

		ImGui::TableNextColumn(); // Move to the first column
		if (ImGui::Selectable(label.c_str())) {
			gbe::Editor::SelectSingle(parent);
		}

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