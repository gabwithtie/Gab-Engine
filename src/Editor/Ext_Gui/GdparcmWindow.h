#pragma once
#include "../Gui/GuiWindow.h"
#include "../Gui/InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>
#include <unordered_map>

namespace gbe {
	namespace editor {
		class GdparcmWindow : public GuiWindow {
			void DrawSelf() override;

			std::array<std::string, 5> scene_names = {
				"scene1",
				"scene2",
				"scene3",
				"scene4",
				"scene5"
			};

			std::unordered_map<std::string, float> scene_progress;

		public:
			std::string GetWindowId() override;
		};
	}
}