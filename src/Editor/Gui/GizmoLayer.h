#pragma once
#include "GuiElement.h"

#include <string>
#include <vector>

namespace gbe {
	class Object;

	namespace editor {
		class GizmoLayer : public GuiElement {
			void DrawSelf() override;
		private:
			bool ext_Begin() override;
			void ext_End() override;
			std::vector<gbe::Object*>& selected;

			bool pointer_here = false;
		public:
			inline bool Get_pointer_here()
			{
				return pointer_here;
			}

			inline GizmoLayer(std::vector<gbe::Object*>& _selected) :
				selected(_selected) {

			}
		};
	}
}