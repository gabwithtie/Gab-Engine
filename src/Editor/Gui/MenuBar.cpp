#include "MenuBar.h"

#include "Engine/gbe_engine.h"
#include "Asset/File/gbeParser.h"
#include "Asset/File/FileUtil.h"

#include "../Editor.h"

#include "../Utility/FileDialogue.h"
#include "../Utility/ModelExport.h"
#include "../Utility/ProjectLoader.h"

void gbe::editor::MenuBar::DrawSelf()
{
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Load Project")) {
			std::string outPath = FileDialogue::GetFilePath(FileDialogue::OPEN, "root.gbe");

			if (outPath.size() != 0) {
				ProjectLoader::Load(outPath);
			}
			else {
				Console::Log("Cancelled File Selection.");
			}
		}
		if (ImGui::MenuItem("Load Scene")) {
			std::string outPath = FileDialogue::GetFilePath(FileDialogue::OPEN, "level");

			if (outPath.size() != 0) {
				gbe::SerializedObject data;
				gbe::asset::serialization::gbeParser::PopulateClass(data, outPath);
				auto newroot = gbe::Engine::CreateBlankRoot(&data);

				gbe::Engine::ChangeRoot(newroot);
			}
			else {
				Console::Log("Cancelled File Selection.");
			}
		}
		if (ImGui::MenuItem("Save Scene")) {
			std::string outPath = FileDialogue::GetFilePath(FileDialogue::SAVE);

			if (outPath.size() != 0) {
				auto data = gbe::Engine::GetCurrentRoot()->Serialize();
				//write
				gbe::asset::serialization::gbeParser::ExportClass(data, outPath);
			}
			else {
				Console::Log("Cancelled File Selection.");
			}
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit")) {
		if (ImGui::MenuItem("Undo")) {
			Editor::Undo();
		}
		if (ImGui::MenuItem("Redo")) {
			Editor::Redo();
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Window")) {
		for (const auto& window : this->windows)
		{
			if (ImGui::MenuItem(window->GetWindowId().c_str())) {
				window->Set_is_open(true);
			}
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Export")) {
		if (ImGui::MenuItem("Scene to obj")) {
			std::string outPath = FileDialogue::GetFilePath(FileDialogue::SAVE);
			std::filesystem::path outpathpath(outPath);
			
			auto newexporter = ModelExport({ Engine::GetCurrentRoot() });
			newexporter.Export(outpathpath);
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
