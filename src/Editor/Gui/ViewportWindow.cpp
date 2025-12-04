#include "ViewportWindow.h"

#include "Graphics/gbe_graphics.h"
#include "Ext/GabVulkan/Objects.h"
#include "imgui_internal.h" // Needed for ImGui::GetCurrentContext()->IO.Framerate

void gbe::editor::ViewportWindow::DrawSelf()
{
	ImVec2 availableSize = ImGui::GetContentRegionAvail();
	Vector2Int cur_resolution = { (int)availableSize.x, (int)availableSize.y };
	ImVec2 content_min = ImGui::GetCursorScreenPos();

	// 1. Handle Resolution and Viewport Update
	if (old_resolution.x != cur_resolution.x || old_resolution.y != cur_resolution.y) {
		old_resolution = cur_resolution;
		RenderPipeline::SetViewportResolution(cur_resolution, { (int)content_min.x, (int)content_min.y });
	}

	// 2. Draw the Viewport Image and Gizmo Layer
	ImGui::Image((ImTextureID)gfx::TextureLoader::GetGuiHandle(this->selected_data), availableSize, { 0, 0 }, { 1, 1 });
	this->gizmoLayer.Draw();

	if (this->gizmoLayer.Get_pointer_here())
		this->pointer_here = false;

	// --- 3. Overlays ---

	// The following code creates overlays on top of the image.
	// We use ImGui::GetWindowDrawList() to draw directly over the window content.
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	// --- Progress Bar Overlay ---
	if (show_progress_bar) {
		float bar_width = 250.0f;
		float bar_height = 15.0f;

		// Calculate the center position of the viewport window
		ImVec2 window_center = ImVec2(content_min.x + availableSize.x * 0.5f, content_min.y + availableSize.y * 0.5f);

		// Calculate the progress bar's top-left position
		ImVec2 bar_pos = ImVec2(window_center.x - bar_width * 0.5f, window_center.y - bar_height * 0.5f);

		// Get the draw list for the current window
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Draw background rect (e.g., dark gray)
		ImU32 bg_color = IM_COL32(50, 50, 50, 200);
		draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_height), bg_color, 4.0f);

		// Draw progress rect (e.g., green/blue)
		ImU32 progress_color = IM_COL32(0, 150, 255, 255);
		float current_progress_width = bar_width * progress_value;
		draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + current_progress_width, bar_pos.y + bar_height), progress_color, 4.0f);

		// Draw border
		ImU32 border_color = IM_COL32(255, 255, 255, 255);
		draw_list->AddRect(bar_pos, ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_height), border_color, 4.0f, ImDrawFlags_None, 1.0f);

		// Draw percentage text in the center
		char text_buf[32];
		snprintf(text_buf, sizeof(text_buf), "%.1f%%", progress_value * 100.0f);
		ImVec2 text_size = ImGui::CalcTextSize(text_buf);
		ImVec2 text_pos = ImVec2(window_center.x - text_size.x * 0.5f, window_center.y - text_size.y * 0.5f);
		draw_list->AddText(text_pos, IM_COL32_WHITE, text_buf);
	}

	// --- FPS Counter Overlay (Bottom Right) ---
	// Requires including "imgui_internal.h" for ImGui::GetCurrentContext()->IO.Framerate
	{
		float frame_rate = ImGui::GetCurrentContext()->IO.Framerate;
		char fps_text[64];
		snprintf(fps_text, sizeof(fps_text), "FPS: %.0f (%.2fms)", frame_rate, 1000.0f / frame_rate);

		ImVec2 text_size = ImGui::CalcTextSize(fps_text);
		float padding = 8.0f;

		// Calculate the position: bottom right of the viewport
		ImVec2 text_pos = ImVec2(
			content_min.x + availableSize.x - text_size.x - padding,
			content_min.y + availableSize.y - text_size.y - padding
		);

		// Draw a semi-transparent background for readability
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 bg_tl = ImVec2(text_pos.x - padding / 2.0f, text_pos.y - padding / 2.0f);
		ImVec2 bg_br = ImVec2(text_pos.x + text_size.x + padding / 2.0f, text_pos.y + text_size.y + padding / 2.0f);
		draw_list->AddRectFilled(bg_tl, bg_br, IM_COL32(0, 0, 0, 150), 3.0f);

		// Draw the FPS text
		draw_list->AddText(text_pos, IM_COL32_WHITE, fps_text);
	}
}