#include "MenuBar.h"

#include "Engine/gbe_engine.h"
#include "Asset/File/gbeParser.h"
#include "Asset/File/FileUtil.h"

#include "../Editor.h"

#include "../Utility/FileDialogue.h"
#include "../Utility/ModelExport.h"

void gbe::editor::MenuBar::DrawSelf()
{
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Load")) {
			std::string outPath = FileDialogue::GetFilePath(FileDialogue::OPEN);

			if (outPath.size() != 0) {
				SerializedObject data;
				gbe::asset::serialization::gbeParser::PopulateClass(data, outPath);
				auto newroot = gbe::Engine::CreateBlankRoot(&data);

				gbe::Engine::ChangeRoot(newroot);
			}
			else {
				Console::Log("Cancelled File Selection.");
			}
		}
		if (ImGui::MenuItem("Save")) {
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
	if (ImGui::BeginMenu("Import")) {
		if (ImGui::MenuItem("Model")) {
			std::string outPath = FileDialogue::GetFilePath(FileDialogue::OPEN);
			std::filesystem::path outpathpath(outPath);
			std::filesystem::path destpathpath("cache/models/");
			std::filesystem::path destmetapath("cache/models/");

			std::string filename = "cache_" + outpathpath.filename().string();
			std::string metafilename = filename + ".gbe";

			destpathpath /= filename;
			destmetapath /= metafilename;

			asset::FileUtil::Copy(outPath, destpathpath);

			auto importdata = asset::data::MeshImportData{
				.path = filename
			};

			asset::serialization::gbeParser::ExportClass(importdata, destmetapath);

			auto newmesh = new asset::Mesh(destmetapath);
			auto material = asset::Material::GetAssetById("lit");

			auto newrenderer = new RenderObject(RenderPipeline::RegisterDrawCall(newmesh, material));
			newrenderer->SetParent(Engine::GetCurrentRoot());
			newrenderer->SetUserCreated();

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;
			newrenderer->World().position.Set(pos);
			newrenderer->SetName("new " + filename);
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
