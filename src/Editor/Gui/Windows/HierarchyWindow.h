#pragma once
#include "GuiWindow.h"
#include "../InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>

#include "Engine/gbe_engine.h"

namespace gbe {
	namespace editor {
		class HierarchyWindow : public GuiWindow {
			void DrawSelf() override;
			std::string GetWindowId() override;
			void DrawChildList(Object* parent, std::string label, unsigned int id = 0);
		};
	}
}