#pragma once

#include "GuiWindow.h"

#include "Graphics/gbe_graphics.h"

#include <string>

namespace gbe {
	namespace editor {
		class CreditsWindow : public GuiWindow {
			std::string GetWindowId() override;

			gbe::asset::Texture* logo_tex;
			TextureData& logo_tex_data;
		public:
			CreditsWindow();
			void DrawSelf() override;
		};
	}
}