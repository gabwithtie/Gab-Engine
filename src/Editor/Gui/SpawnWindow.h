#pragma once

#include "GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {
		class SpawnWindow : public GuiWindow {
			void DrawSelf() override;
			std::string GetWindowId() override;
		public:
			std::vector<gbe::Object*>* selected;
		};
	}
}