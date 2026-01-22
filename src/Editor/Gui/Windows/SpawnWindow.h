#pragma once

#include "GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {
		class SpawnWindow : public GuiWindow {
			void DrawSelf() override;
			std::string GetWindowId() override;
			std::vector<gbe::Object*>& selected;
		public:
			inline SpawnWindow(std::vector<gbe::Object*>& _selected) :
				selected(_selected)
			{

			}
		};
	}
}