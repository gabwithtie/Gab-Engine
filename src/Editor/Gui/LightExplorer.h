#pragma once
#include "GuiWindow.h"
#include "InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>

#include "Engine/gbe_engine.h"

namespace gbe {
	namespace editor {
		class LightExplorer : public GuiWindow {
			void DrawSelf() override;
			void DrawChildList(Object* parent, std::string label, unsigned int id = 0);
		public:
			std::string GetWindowId() override;
		};
	}
}