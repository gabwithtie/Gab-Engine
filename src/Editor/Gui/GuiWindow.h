#pragma once

#include "GuiElement.h"

#include <typeinfo>
#include <string>

#include <imgui.h>

namespace gbe {
	namespace editor {
		class GuiWindow : public GuiElement{
		protected:
			virtual std::string GetWindowId() = 0;
		private:
			bool ext_Begin() override;
			void ext_End() override;
		};
	}
}