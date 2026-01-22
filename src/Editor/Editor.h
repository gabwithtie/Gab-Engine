#pragma once

#include "Engine/gbe_engine.h"

#include "Gui/InspectorData.h"
#include "Gui/MenuBar.h"
#include "Gui/ContextMenus.h"
#include "Gui/Windows/CreditsWindow.h"
#include "Gui/Windows/InspectorWindow.h"
#include "Gui/Windows/SpawnWindow.h"
#include "Gui/Windows/StateWindow.h"
#include "Gui/Windows/HierarchyWindow.h"
#include "Gui/Windows/ImageDebugger.h"
#include "Gui/Windows/ConsoleWindow.h"
#include "Gui/Windows/ViewportWindow.h"
#include "Gui/Windows/LightExplorer.h"

namespace gbe {
	class RenderPipeline;
	class Engine;
	class Window;

	class Editor {
	private:
		static Editor* instance;

		Window* mwindow;
		RenderPipeline* mrenderpipeline;
		Time* mtime;

		std::vector<gbe::Object*> selected;

		struct EditorAction {
			std::function<void()> action_done;
			std::function<void()> undo;
		};
		std::vector<EditorAction> action_stack;
		unsigned int cur_action_index = 0;

		bool pointer_inUi;
		bool keyboard_inUi;
		bool keyboard_shifting = false;

		//RECORDING
		bool is_recording = false;

		//============GUI===========//
		bool gui_initialized = false;

		//DOCKS
		editor::MenuBar menubar;

		//WINDOWS
		editor::HierarchyWindow hierarchyWindow;
		editor::InspectorWindow inspectorwindow;
		editor::SpawnWindow spawnWindow;
		editor::StateWindow stateWindow;
		editor::ConsoleWindow consoleWindow;
		editor::ImageDebugger imageDebuggerWindow;
		editor::ViewportWindow viewportWindow;
		editor::LightExplorer lightWindow;

		std::vector<editor::GuiWindow*> windows = {
			&hierarchyWindow,
			&inspectorwindow,
			&spawnWindow,
			&stateWindow,
			&consoleWindow,
			&imageDebuggerWindow,
			&viewportWindow,
			&lightWindow
		};

	public:
		Editor(RenderPipeline* renderpipeline, Window* window, Time* _mtime, std::vector<editor::GuiWindow*> additionalwindows);
		~Editor();
		static void OnDeselect(Object* other);
		static void SelectSingle(Object* other);
		static void UpdateSelection();
		static void DeselectAll();
		static void CommitAction(std::function<void()> redo, std::function<void()> undo);
		static void Undo();
		static void Redo();
		void PrepareSceneChange();
		void PrepareUpdate();
		void ProcessRawWindowEvent(void* rawwindowevent);
		void RenderPass();
		inline bool FocusedOnEditorUI() {
			bool pointer_really_inUi = pointer_inUi && !viewportWindow.Get_pointer_here();
			return pointer_really_inUi || keyboard_inUi;
		}
	};
}