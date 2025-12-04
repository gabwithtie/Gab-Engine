#pragma once

#include "GuiWindow.h"
#include "GizmoLayer.h"

#include "Engine/gbe_engine.h"

#include <string>
#include <algorithm>

namespace gbe {
	namespace editor {

		class ViewportWindow : public GuiWindow {
			void DrawSelf() override;

			TextureData* selected_data = nullptr;
			Vector2Int old_resolution;

			editor::GizmoLayer gizmoLayer;

			// --- Added for Progress Bar ---
			bool show_progress_bar = false;
			float progress_value = 0.0f; // Value between 0.0f and 1.0f
			// ------------------------------

		protected:
			inline void push_styles() override {
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f , 0.0f });
			}
			inline void pop_styles() override {
				ImGui::PopStyleVar();
			}
		public:
			inline std::string GetWindowId() override {
				return "ViewportWindow";
			}
			inline ViewportWindow(std::vector<gbe::Object*>& _selected) :
				gizmoLayer(_selected)
			{
				selected_data = &gfx::TextureLoader::GetDataMap().at("mainpass");
			}

			// --- Added for Progress Bar Functionality ---
			inline void toggle_progress_bar(bool show) {
				show_progress_bar = show;
			}
			inline void set_progress(float value) {
				// Clamp the value between 0.0f and 1.0f
				progress_value = std::fmax(0.0f, std::fmin(1.0f, value));
			}
			// --------------------------------------------
		};
	}
}