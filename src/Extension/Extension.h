#pragma once

#include <vector>

namespace gbe {
	namespace editor {
		class GuiWindow;
	}

	class Extension {
	public:
		std::vector<editor::GuiWindow*> extension_windows;
		virtual void OnEngineInitialize() = 0;
		virtual void OnEngineRunLoopStart() = 0;
		virtual void OnEngineRunLoopEnd() = 0;
		virtual void OnEngineShutdown() = 0;
	};
}