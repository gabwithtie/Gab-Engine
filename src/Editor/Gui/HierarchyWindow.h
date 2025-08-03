#pragma once
#include "GuiWindow.h"
#include "InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {

		class HierarchyWindow : public GuiWindow {
			void DrawSelf() override;
			std::string GetWindowId() override;
			void DrawChildList(Object* parent, std::string label, unsigned int id = 0);
		public:
			Time* mtime;
			Root* root;
		};
	}
}