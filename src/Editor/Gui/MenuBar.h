#pragma once

#include "GuiElement.h"

#include <string>

#include <imgui.h>
#include <list>

namespace gbe {
	namespace editor {
		class GuiWindow;

		class MenuBar : public GuiElement{
		private:
			bool ext_Begin() override;
			void ext_End() override;

			std::list<GuiWindow*>& windows;
		public:
			void DrawSelf() override;
			
			inline MenuBar(std::list<GuiWindow*>& _windows) : windows(_windows) {
			}
		};
	}
}