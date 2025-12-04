#include "GdparcmWindow.h"

#include "Ext/Gdparcm/MeshAsync.h"

namespace gbe {
	namespace editor {
		void GdparcmWindow::DrawSelf()
		{
			ImGui::SeparatorText("GDP ARCM Explorer");

			if (ImGui::Button("Load All")) {
				for (const auto meshloader : gdparcm::MeshAsync::Get_active_mesh_requests())
				{
					meshloader->Load();
				}
			}

			if (ImGui::Button("Unload All")) {
				for (const auto meshloader : gdparcm::MeshAsync::Get_active_mesh_requests())
				{
					meshloader->Unload();
				}
			}

			if (ImGui::BeginTable("GDPARCMTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
			{
				ImGui::TableSetupColumn("Image");
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Progress");
				ImGui::TableHeadersRow();

				bool any_in_progress = false;
				float lowest_progress = 10.0f;

				for (const auto meshloader : gdparcm::MeshAsync::Get_active_mesh_requests())
				{
					bool is_clicked = false;

					ImGui::PushID(std::to_string(meshloader->Get_id()).c_str());
					ImGui::TableNextRow(); // Start a new row
					// Image Column
					ImGui::TableNextColumn();

					{
						const auto& image = gbe::asset::Texture::GetAssetById(meshloader->GetName());
						auto& image_data = gbe::TextureLoader::GetAssetData(image);
						ImGui::Image((ImTextureID)gbe::gfx::TextureLoader::GetGuiHandle(&image_data), ImVec2(100, 100), ImVec2(1, 1), ImVec2(0, 0));
					}

					// Name Column
					ImGui::TableNextColumn();

					if (ImGui::Selectable(meshloader->GetName().c_str(), is_clicked)) {
						for (const auto other_ml : gdparcm::MeshAsync::Get_active_mesh_requests())
						{
							other_ml->Hide();
						}
						
						meshloader->Load();
					}

					// Progress Column
					ImGui::TableNextColumn();

					ImGui::ProgressBar(meshloader->Get_progress(), ImVec2(-1.0f, 0.0f), nullptr);

					if(meshloader->Get_progress() > 0.01f && meshloader->Get_progress() < 0.95f) {
						any_in_progress = true;
						
						if (meshloader->Get_progress() < lowest_progress) {
							lowest_progress = meshloader->Get_progress();
						}
					}

					ImGui::PopID();
				}

				ImGui::EndTable();

				if (any_in_progress && lowest_progress < 0.95f) {
					this->viewportwindow.set_progress(lowest_progress);
					this->viewportwindow.toggle_progress_bar(true);
				}
				else {
					this->viewportwindow.toggle_progress_bar(false);
				}
			}
		}
		std::string GdparcmWindow::GetWindowId()
		{
			return "GDP ARCM Explorer";
		}
	}
}