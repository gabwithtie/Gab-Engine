#pragma once

#include "Engine/gbe_engine.h"

#include "Ext/GabVulkan/Objects.h"

#include "Gui/InspectorData.h"
#include "Gui/CreditsWindow.h"
#include "Gui/InspectorWindow.h"
#include "Gui/SpawnWindow.h"
#include "Gui/StateWindow.h"
#include "Gui/HierarchyWindow.h"
#include "Gui/ImageDebugger.h"
#include "Gui/ConsoleWindow.h"
#include "Gui/MenuBar.h"
#include "Gui/GizmoLayer.h"

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

		//DOCKS
		editor::MenuBar menubar;

		//WINDOWS
		editor::HierarchyWindow hierarchyWindow;
		editor::InspectorWindow inspectorwindow;
		editor::SpawnWindow spawnWindow;
		editor::StateWindow stateWindow;
		editor::ConsoleWindow consoleWindow;
		editor::ImageDebugger imageDebuggerWindow;

		std::list<editor::GuiWindow*> windows = {
			&hierarchyWindow,
			&inspectorwindow,
			&spawnWindow,
			&stateWindow,
			&consoleWindow,
			&imageDebuggerWindow
		};

		//LAYERS
		editor::GizmoLayer gizmoLayer;


		//DYNAMICALLY ALLOCATED
		vulkan::DescriptorPool* gui_ds;

	public:
		Editor(RenderPipeline* renderpipeline, Window* window, Time* _mtime);
		~Editor();
		static void SelectSingle(Object* other);
		static void DeselectAll();
		static void CommitAction(std::function<void()> redo, std::function<void()> undo);
		static void Undo();
		static void Redo();
		void PrepareSceneChange();
		void UpdateSelectionGui(Object* newlyclicked);
		void PrepareUpdate();
		void ProcessRawWindowEvent(void* rawwindowevent);
		void RenderPass(vulkan::CommandBuffer* cmd);
		inline bool FocusedOnEditorUI() {
			return pointer_inUi || keyboard_inUi;
		}
	};
}