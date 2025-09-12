#pragma once

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl2.h>

#include <ImGuizmo.h>

#include "Engine/gbe_engine.h"

#include "Ext/GabVulkan/Objects.h"

#include "Gui/InspectorData.h"
#include "Gui/CreditsWindow.h"
#include "Gui/InspectorWindow.h"
#include "Gui/SpawnWindow.h"
#include "Gui/StateWindow.h"
#include "Gui/HierarchyWindow.h"
#include "Gui/ConsoleWindow.h"
#include "Gui/MenuBar.h"

namespace gbe {
	class RenderPipeline;
	class Engine;
	class Window;

	class Editor {
	private:
		static Editor* instance;

		Window* mwindow;
		Engine* mengine;
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

		//BOX GIZMOS
		asset::Material* gizmo_box_mat;
		std::unordered_map<gbe::Object*, RenderObject*> gizmo_boxes;

		//WINDOWS
		editor::HierarchyWindow* hierarchyWindow;
		editor::InspectorWindow* inspectorwindow;
		editor::SpawnWindow* spawnWindow;
		editor::StateWindow* stateWindow;
		editor::ConsoleWindow* consoleWindow;
		editor::MenuBar* menubar;
	public:
		Editor(RenderPipeline* renderpipeline, Window* window, Engine* engine, Time* _mtime);
		static void SelectSingle(Object* other);
		static void DeselectAll();
		static void RegisterAction(std::function<void()> redo, std::function<void()> undo);
		static void Undo();
		static void Redo();
		void PrepareSceneChange();
		void UpdateSelectionGui(Object* newlyclicked);
		void PrepareFrame();
		void Update();
		void ProcessRawWindowEvent(void* rawwindowevent);
		void PresentFrame();
		void RenderPass(vulkan::CommandBuffer* cmd);
		void CreateGizmoBox(gbe::RenderObject* boxed, gbe::Object* rootboxed);
	};
}