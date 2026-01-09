#pragma once

#include "Editor/Gui/InspectorData.h"
#include "Editor/Gui/GuiWindow.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {
		class AnitoBuilderWindow : public GuiWindow {
			void DrawSelf() override;
			void DrawChildList(Object* parent, std::string label, unsigned int id = 0);
		public:
			std::string GetWindowId() override;
		};
	}
}