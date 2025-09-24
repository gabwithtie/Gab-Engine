#include "ImageDebugger.h"

#include "Graphics/gbe_graphics.h"

void gbe::editor::ImageDebugger::DrawSelf()
{
	if (ImGui::BeginCombo("Image", cur_image_id.c_str()))
	{
		auto& map = gfx::TextureLoader::GetDataMap();

		for (auto& pair : map)
		{
			auto cur_id = pair.first;

			const bool is_selected = (cur_id == cur_image_id.c_str());
			if (ImGui::Selectable(cur_id.c_str(), is_selected))
			{
				selected_data = &pair.second;
			}

			// Set the initial focus when opening the combo (scrolling to the item if needed)
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (selected_data != nullptr) {
		ImGui::Image(gfx::TextureLoader::GetGuiHandle(this->selected_data), {512, 512}, {0, 1}, {1, 0});
	}
}