#include "GdparcmWindow.h"

#include "Ext/Gdparcm/MeshAsync.h"

namespace gbe {
	namespace editor {
		void GdparcmWindow::DrawSelf()
		{
			ImGui::SeparatorText("GDP ARCM Explorer");

			if (ImGui::Button("Reload All")) {
				for (const auto meshloader : gdparcm::MeshAsync::Get_active_mesh_requests())
				{
					meshloader->Reload();
				}
			}

			if (ImGui::BeginTable("GDPARCMTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
			{
				ImGui::TableSetupColumn("Image");
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Progress");
				ImGui::TableHeadersRow();

				for (const auto meshloader : gdparcm::MeshAsync::Get_active_mesh_requests())
				{
					ImGui::PushID(meshloader->GetName().c_str());
				
					ImGui::TableNextRow(); // Start a new row
					// Image Column
					ImGui::TableNextColumn();
					// Placeholder for image, replace with actual image rendering if available
					ImGui::Text("[Image]");
					// Name Column
					ImGui::TableNextColumn();
					ImGui::Text("%s", meshloader->GetName().c_str());
					// Progress Column
					ImGui::TableNextColumn();

					ImGui::ProgressBar(meshloader->Get_progress(), ImVec2(-1.0f, 0.0f), nullptr);

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		}
		std::string GdparcmWindow::GetWindowId()
		{
			return "GDP ARCM Explorer";
		}
	}
}