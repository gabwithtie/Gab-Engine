#pragma once
#include "../Gui/GuiWindow.h"
#include "../Gui/InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>
#include <unordered_map>

#include "../Gui/ViewportWindow.h"

namespace gbe {
	namespace editor {
		class GdparcmWindow : public GuiWindow {
			void DrawSelf() override;

			gbe::editor::ViewportWindow& viewportwindow;
		public:
			inline GdparcmWindow(gbe::editor::ViewportWindow& _viewportwindow) :
				viewportwindow(_viewportwindow){

			}

			std::string GetWindowId() override;
		};
	}
}