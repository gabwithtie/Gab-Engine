#pragma once

#include "GuiWindow.h"
#include "../GizmoLayer.h"

#include <string>
#include "Math/gbe_math.h"

namespace gbe {
	namespace gfx
	{
		class TextureData;
	}

	class Object;

	namespace editor {

		class ViewportWindow : public GuiWindow {
			void DrawSelf() override;

			gfx::TextureData* selected_data = nullptr;
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
			ViewportWindow(std::vector<gbe::Object*>& _selected);
		};
	}
}