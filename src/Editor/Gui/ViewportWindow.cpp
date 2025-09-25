#include "ViewportWindow.h"

#include "Graphics/gbe_graphics.h"
#include "Ext/GabVulkan/Objects.h"

void gbe::editor::ViewportWindow::DrawSelf()
{
	ImVec2 availableSize = ImGui::GetContentRegionAvail();
	Vector2Int cur_resolution = { (int)availableSize.x, (int)availableSize.y };

	if (old_resolution.x != cur_resolution.x || old_resolution.y != cur_resolution.y) {
		old_resolution = cur_resolution;
		RenderPipeline::SetViewportResolution(cur_resolution);
	}

	ImGui::Image(gfx::TextureLoader::GetGuiHandle(this->selected_data), availableSize, { 0, 1 }, { 1, 0 });
}