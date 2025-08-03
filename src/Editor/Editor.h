#pragma once

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl2.h>

#include "Engine/gbe_engine.h"

#include "Gui/InspectorData.h"
#include "Gui/CreditsWindow.h"
#include "Gui/ColorpickerWindow.h"
#include "Gui/InspectorWindow.h"
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

		bool pointer_inUi;
		bool keyboard_inUi;
		bool keyboard_shifting = false;

		//RECORDING
		bool is_recording = false;


		//GIZMO BOX CACHE
		asset::Material* gizmo_box_mat;
		//GIZMO ARROW CACHE
		asset::Mesh* gizmo_arrow_mesh;
		DrawCall* gizmo_arrow_drawcall_r;
		DrawCall* gizmo_arrow_drawcall_g;
		DrawCall* gizmo_arrow_drawcall_b;

		//ARROW GIZMOS
		float gizmo_fixed_depth = 10.0f;
		float gizmo_offset_distance = 1.0f;
		Vector3 current_selected_position;
		Vector3 original_selected_position;
		Vector3 selected_f;
		Vector3 selected_r;
		Vector3 selected_u;

		PhysicsObject* f_gizmo = nullptr;
		PhysicsObject* r_gizmo = nullptr;
		PhysicsObject* u_gizmo = nullptr;

		PhysicsObject* held_gizmo = nullptr;

		//BOX GIZMOS
		std::unordered_map<gbe::Object*, RenderObject*> gizmo_boxes;

		std::array<PhysicsObject**, 3> gizmo_arrows = {
			&f_gizmo,
			&r_gizmo,
			&u_gizmo
		};

		//WINDOWS
		editor::InspectorWindow* inspectorwindow;
		editor::MenuBar* menubar	;
	public:
		Editor(RenderPipeline* renderpipeline, Window* window, Engine* engine, Time* _mtime);
		static void SelectSingle(Object* other);
		void DeselectAll();
		void PrepareSceneChange();
		void UpdateSelectionGui(Object* newlyclicked);
		void PrepareFrame();
		void DrawFrame();
		void ProcessRawWindowEvent(void* rawwindowevent);
		void PresentFrame();
		void RenderPass(VkCommandBuffer cmd);
		void CreateGizmoArrow(gbe::PhysicsObject*& out_g, DrawCall* drawcall, Vector3 rotation, Vector3 direction);
		void CreateGizmoBox(gbe::RenderObject* boxed, gbe::Object* rootboxed);
	};
}