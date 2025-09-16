#pragma once

#include "GuiWindow.h"
#include "InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {

		class InspectorWindow : public GuiWindow {
			void DrawSelf() override;
			std::string GetWindowId() override;
			bool DrawVector3Field(std::string label, Vector3* field, bool x_interactable = true, bool y_interactable = true, bool z_interactable = true);

			bool is_reparenting = false;
			gbe::Object* reparentee;
			std::vector<gbe::Object*>& selected;
		public:
			inline InspectorWindow(std::vector<gbe::Object*>& _selected):
				selected(_selected)
			{

			}
		};
	}
}