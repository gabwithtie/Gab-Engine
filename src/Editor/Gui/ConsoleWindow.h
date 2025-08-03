#pragma once
#include "GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {

		class ConsoleWindow : public GuiWindow {
			std::vector<std::string> logs;

			void DrawSelf() override;
			std::string GetWindowId() override;
			void receive_log(std::string);
		public:
			ConsoleWindow();
		};
	}
}