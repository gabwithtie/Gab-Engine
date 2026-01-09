#pragma once
#include "GuiWindow.h"

#include <string>
#include <vector>

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