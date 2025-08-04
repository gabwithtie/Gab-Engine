#pragma once

#include "GuiElement.h"

#include <string>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl2.h>

namespace gbe {
	namespace editor {
		class MenuBar : public GuiElement{
		public:
			void DrawSelf() override;
		private:
			bool ext_Begin() override;
			void ext_End() override;
		};
	}
}