#include "MenuBar.h"

#include "Engine/gbe_engine.h"
#include "Asset/Parsing/gbeParser.h"

void gbe::editor::MenuBar::DrawSelf()
{
	if (ImGui::BeginMenu("Serialization")) {
		if (ImGui::MenuItem("Load Serialized File")) {
			SerializedObject data;
			gbe::asset::serialization::gbeParser::PopulateClass(data, "out/default.level");;
			auto newroot = gbe::Engine::CreateBlankRoot();
			newroot->Deserialize(data);
			Engine::ChangeRoot(newroot);
		}
		if (ImGui::MenuItem("Write Serialized File")) {
			auto data = gbe::Engine::GetCurrentRoot()->Serialize();
			//write
			gbe::asset::serialization::gbeParser::ExportClass(data, "out/default.level");
		}
		ImGui::EndMenu();
	}
}

bool gbe::editor::MenuBar::ext_Begin()
{
	if (ImGui::BeginMainMenuBar()) {
		return true;
	}
	else {
		return false;
	}
}

void gbe::editor::MenuBar::ext_End()
{
	ImGui::EndMainMenuBar();
}
