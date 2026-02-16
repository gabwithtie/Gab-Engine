#pragma once

#include "GuiWindow.h"
#include "../InspectorData.h"

#include "Engine/gbe_engine.h"

#include <string>
#include <unordered_map>

namespace gbe {
	namespace editor {

		class InspectorWindow : public GuiWindow {
			void DrawSelf() override;
			void DrawFieldLabel(std::string label);

			std::unordered_map<void*, InspectorData*> data;
		public:
			std::string GetWindowId() override;
			void SetInspectorData(std::unordered_map<void*, InspectorData*> _data);
			inline InspectorData* GetData(void* owner) {
				auto it = this->data.find(owner);

				if (it == this->data.end())
					return nullptr;

				return it->second;
			}
		};
	}
}