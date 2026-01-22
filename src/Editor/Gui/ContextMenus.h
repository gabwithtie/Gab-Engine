#pragma once

#include <imgui.h>
#include <unordered_map>
#include <string>
#include <functional>

#include "../Editor.h"

namespace gbe::editor {
	class ContextMenus {
	public:
		static void GenericObject(gbe::Object* obj);
	};
}