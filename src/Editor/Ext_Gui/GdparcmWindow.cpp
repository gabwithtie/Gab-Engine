#include "GdparcmWindow.h"

namespace gbe {
	namespace editor {
		void GdparcmWindow::DrawSelf()
		{
			ImGui::SeparatorText("GDP ARCM Explorer");
			if (ImGui::BeginTable("GDPARCMTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable))
			{
				ImGui::TableSetupColumn("Image");
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Progress");
				ImGui::TableHeadersRow();

				for (const auto scenename : this->scene_names)
				{
					ImGui::PushID(scenename.c_str());
				
					ImGui::TableNextRow(); // Start a new row
					// Image Column
					ImGui::TableNextColumn();
					// Placeholder for image, replace with actual image rendering if available
					ImGui::Text("[Image]");
					// Name Column
					ImGui::TableNextColumn();
					ImGui::Text("%s", scenename.c_str());
					// Progress Column
					ImGui::TableNextColumn();
					float progress = 0;

					if(this->scene_progress.find(scenename) != this->scene_progress.end())
					{
						progress = this->scene_progress[scenename];
					}
					ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), nullptr);

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