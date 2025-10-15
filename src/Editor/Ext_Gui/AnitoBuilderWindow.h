#pragma once
#include "../Gui/GuiWindow.h"
#include "../Gui/InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {
		class AnitoBuilderWindow : public GuiWindow {
			void DrawSelf() override;
			std::string GetWindowId() override;
			void DrawChildList(Object* parent, std::string label, unsigned int id = 0);
		};
	}
}