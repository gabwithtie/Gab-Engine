#pragma once
#include "GuiElement.h"

#include "Engine/gbe_engine.h"

#include <string>

namespace gbe {
	namespace editor {
		class GizmoLayer : public GuiElement {
			void DrawSelf() override;
		private:
			bool ext_Begin() override;
			void ext_End() override;
			std::vector<gbe::Object*>& selected;
		public:
			inline GizmoLayer(std::vector<gbe::Object*>& _selected) :
				selected(_selected) {

			}
		};
	}
}