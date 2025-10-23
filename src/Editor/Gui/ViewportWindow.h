#pragma once

#include "GuiWindow.h"
#include "GizmoLayer.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {

		class ViewportWindow : public GuiWindow {
			void DrawSelf() override;

			TextureData* selected_data = nullptr;
			Vector2Int old_resolution;

			editor::GizmoLayer gizmoLayer;
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
		};
	}
}