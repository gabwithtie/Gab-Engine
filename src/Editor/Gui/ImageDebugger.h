#pragma once

#include "GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {

		class ImageDebugger : public GuiWindow {
			void DrawSelf() override;
			inline std::string GetWindowId() override {
				return "ImageDebugger";
			}

			std::string cur_image_id = "";
			TextureData* selected_data = nullptr;
		public:
			inline ImageDebugger()
			{

			}
		};
	}
}