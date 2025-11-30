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

			

		public:
			std::string GetWindowId() override;
		};
	}
}