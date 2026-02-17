#pragma once

#include "GuiWindow.h"
#include <string>

namespace gbe {
	class Editor;

	namespace editor {

		class TexturePainterWindow : public GuiWindow {
			void DrawSelf() override;
		public:
			inline std::string GetWindowId() override {
				return "TexturePainter";
			}
			void Set_is_open(bool newstate) override;
			TexturePainterWindow();
		};
	}
}