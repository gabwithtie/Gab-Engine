#pragma once

#include "GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {

		class ViewportWindow : public GuiWindow {
			void DrawSelf() override;
			inline std::string GetWindowId() override {
				return "ViewportWindow";
			}

			TextureData* selected_data = nullptr;
			Vector2Int old_resolution;

		public:
			inline ViewportWindow() {
				selected_data = &gfx::TextureLoader::GetDataMap().at("mainpass");
			}
		};
	}
}