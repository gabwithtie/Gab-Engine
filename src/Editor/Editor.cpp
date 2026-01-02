#include "Editor.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include "Ext/bgfx-imgui/imgui_impl_bgfx.h"

#include <bx/bx.h>
#include <bx/allocator.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <ImGuizmo.h>

#include <typeinfo>

#include "Graphics/gbe_graphics.h"
#include "Engine/gbe_engine.h"
#include "Physics/gbe_physics.h"

#include "Ext/GabVulkan/Objects.h"

gbe::Editor* gbe::Editor::instance = nullptr;

gbe::Editor::Editor(RenderPipeline* renderpipeline, Window* window, Time* _mtime):
	menubar(this->windows),

	spawnWindow(this->selected),
	inspectorwindow(this->selected),
	viewportWindow(this->selected)
{
	instance = this;

	this->mwindow = window;
	this->mrenderpipeline = renderpipeline;
	this->mtime = _mtime;

	//===========================IMGUI=============================//
	SDL_Window* implemented_window = static_cast<SDL_Window*>(window->Get_implemented_window());

	ImGui::CreateContext();

	ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
	ImGui_ImplSDL2_InitForD3D(implemented_window);
#elif BX_PLATFORM_OSX
	ImGui_ImplSDL2_InitForMetal(implemented_window);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_EMSCRIPTEN
	ImGui_ImplSDL2_InitForOpenGL(implemented_window, nullptr);
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
	// BX_PLATFORM_EMSCRIPTEN

	//IO FLAGS
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	//IO FEEL
	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 1.0f;
	style.FrameRounding = 4.0f;
	style.WindowRounding = 4.0f;
	style.ChildRounding = 4.0f;
	style.PopupRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;
	style.ScrollbarSize = 10.0f;
	style.ScrollbarRounding = 12.0f;
	style.WindowBorderSize = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.IndentSpacing = 20.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.FramePadding = ImVec2(6.0f, 4.0f);

	// Normalized R, G, B, A values [0.0f to 1.0f]
	ImVec4* colors = style.Colors;

	//COLOR CACHING
	auto green = ImVec4(0.47f, 0.56f, 0.28f, 1.00f);
	auto green_b = ImVec4(0.40f, 0.50f, 0.23f, 1.00f);

	// Main background colors
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f); // Main Editor Background
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(green.x, green.y, green.z, 0.70f); // Subtle Border

	// Interactive element backgrounds (Input fields, sliders, etc.)
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // Slightly lighter than WindowBg
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.28f, 0.35f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(green.x, green.y, green.z, 1.00f);

	// Title Bars
	colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.10f, 0.12f, 0.51f);

	// Accent Colors (Using a desaturated blue for sleekness)
	ImVec4 accentColor = ImVec4(green.x, green.y, green.z, 1.00f); // A standard medium blue
	ImVec4 activeColor = ImVec4(green_b.x, green_b.y, green_b.z, 1.00f); // Darker blue for active state

	// Buttons/Headers
	colors[ImGuiCol_Button] = colors[ImGuiCol_FrameBg];
	colors[ImGuiCol_ButtonHovered] = accentColor;
	colors[ImGuiCol_ButtonActive] = activeColor;

	colors[ImGuiCol_Header] = colors[ImGuiCol_FrameBg];
	colors[ImGuiCol_HeaderHovered] = accentColor;
	colors[ImGuiCol_HeaderActive] = activeColor;

	// Tab Bar (Crucial for game engine editors)
	colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.19f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(green.x, green.y, green.z, 1.00f); // Active tab is bright
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // Tab body color
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.17f, 0.19f, 0.22f, 1.00f);

	// Slider Grab and Checkmark
	colors[ImGuiCol_SliderGrab] = accentColor;
	colors[ImGuiCol_SliderGrabActive] = activeColor;
	colors[ImGuiCol_CheckMark] = accentColor;

	// Resize Grip
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.22f, 0.27f, 0.50f);
	colors[ImGuiCol_ResizeGripHovered] = accentColor;
	colors[ImGuiCol_ResizeGripActive] = activeColor;
}

gbe::Editor::~Editor()
{
	delete gui_ds;
}

void gbe::Editor::SelectSingle(Object* other) {
	if(other->GetEditorFlag(Object::SELECT_PARENT_INSTEAD)) {
		if (other->GetParent() != nullptr) {
			SelectSingle(other->GetParent());
			return;
		}
		else
			throw std::runtime_error("Object has SELECT_PARENT_INSTEAD flag but no parent.");
	}

	auto newlyclicked = other;
	RenderObject* renderer_has = nullptr;

	other->CallRecursively([&](Object* child) {
		auto renderer_check = dynamic_cast<RenderObject*>(child);

		if (renderer_check != nullptr) {
			renderer_has = renderer_check;
		}
		});


	bool deselection = false;

	if (instance->keyboard_shifting) {
		auto it = std::find(instance->selected.begin(), instance->selected.end(), other);

		// DESELECT IF FOUND
		if (it != instance->selected.end()) {
			instance->selected.erase(it);

			deselection = true;
		}
	}
	else { //CLEAR SELECTION IF NOT MULTISELECTING AND CLICKED SOMETHING ELSE
		instance->DeselectAll();
	}

	if (!deselection) {
		//SELECT AND BOX
		instance->selected.push_back(other);
	}
}

void gbe::Editor::DeselectAll() {
	instance->selected.clear();
}

void gbe::Editor::CommitAction(std::function<void()> action_done, std::function<void()> undo)
{
	action_done();

	EditorAction newaction = {
		.action_done = action_done,
		.undo = undo
	};

	for (size_t i = instance->action_stack.size() - instance->cur_action_index; i > 0; i--)
	{
		instance->action_stack.pop_back();
	}

	instance->action_stack.push_back(newaction);
	instance->cur_action_index = instance->action_stack.size();
}

void gbe::Editor::Undo()
{
	if (instance->cur_action_index == 0)
		return;

	instance->cur_action_index--;
	instance->action_stack[instance->cur_action_index].undo();
}

void gbe::Editor::Redo()
{
	if (instance->cur_action_index == instance->action_stack.size())
		return;

	instance->action_stack[instance->cur_action_index].action_done();
	instance->cur_action_index++;
}

void gbe::Editor::ProcessRawWindowEvent(void* rawwindowevent) {
	auto sdlevent = static_cast<SDL_Event*>(rawwindowevent);

	ImGui_ImplSDL2_ProcessEvent(sdlevent);

	//CHECK SHIFT CLICK
	if (sdlevent->key.keysym.sym == SDLK_LSHIFT) {
		if (sdlevent->type == SDL_KEYDOWN) {
			this->keyboard_shifting = true;
		}
		else if (sdlevent->type == SDL_KEYUP) {
			this->keyboard_shifting = false;
		}
	}

	//CLICKED
	if (sdlevent->type == SDL_MOUSEBUTTONDOWN) {
		if (sdlevent->button.button == SDL_BUTTON_LEFT && !this->FocusedOnEditorUI()) {

			//RAYCAST MECHANICS
			auto current_camera = Engine::GetActiveCamera();
			Vector3 camera_pos = current_camera->World().position.Get();
			auto mousedir = current_camera->ScreenToRay(mwindow->GetMouseDecimalPos());

			//OBJECT SELECTION
			Vector3 ray_dir = mousedir * 10000.0f;
			auto result = physics::RaycastAll(camera_pos, ray_dir);

			if (!result.result) { //NOTHING WAS CLICKED
				if (!this->keyboard_shifting) { //NOT MULTISELECTING
					//CLEAR SELECTION IF NOT MULTISELECTING AND CLICKED NOTHING
					this->selected.clear();
				}
			}
			else {
				Object* closest_obj = nullptr;
				float sqrdist_of_closest = INFINITY;
				bool gizmo_in_ray = false;

				const auto CheckClosest = [&](Object* obj) {
					if (!Object::ValidateObject(obj))
						return;
					Vector3 toobj = obj->World().position.Get() - camera_pos;
					float curdist = toobj.SqrMagnitude();
					if (curdist < sqrdist_of_closest) {
						sqrdist_of_closest = curdist;
						closest_obj = obj;
					}
					};

				//Look for gizmo, otherwise, just select closest valid object
				for (size_t i = 0; i < result.others.size(); i++)
				{
					auto objinray = result.others[i];

					if (!Object::ValidateObject(objinray))
						continue;

					CheckClosest(objinray);
				}
				
				if (closest_obj == nullptr)
					return;

				SelectSingle(closest_obj);
			}
		}
	}
	if (sdlevent->type == SDL_MOUSEBUTTONUP) {
		if (sdlevent->button.button == SDL_BUTTON_LEFT) {
			//COMMIT HELD GIZMO ACTION
			
		}
	}
}

void gbe::Editor::PrepareSceneChange() {
	this->DeselectAll();
}

void gbe::Editor::UpdateSelectionGui(Object* newlyclicked) {
	
}

void gbe::Editor::PrepareUpdate()
{
	//imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	auto& ui_io = ImGui::GetIO();

	this->pointer_inUi = ui_io.WantCaptureMouse;
	this->keyboard_inUi = ui_io.WantCaptureKeyboard;

	//==============================EDITOR UPDATE==============================//
	if (selected.size() == 1) {
		auto current_camera = Engine::GetActiveCamera();
		Vector3 camera_pos = current_camera->World().position.Get();
		auto mousedir = current_camera->ScreenToRay(mwindow->GetMouseDecimalPos());
	}

	//==============================IMGUI==============================//
	ImGuiID dockspace_id = ImGui::GetID("maindockspace");
	ImGui::DockSpaceOverViewport(dockspace_id);

	if (!gui_initialized) {
		gui_initialized = true;

		ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size); // Set size

		ImGuiID r = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
		ImGuiID l = dockspace_id;
		ImGuiID r_d = ImGui::DockBuilderSplitNode(r, ImGuiDir_Down, 0.5f, nullptr, &r);
		ImGuiID r_u = r;

		// Dock windows into the new nodes
		this->viewportWindow.Set_is_open(true);
		this->inspectorwindow.Set_is_open(true);
		this->anitobuilderWindow.Set_is_open(true);
		this->lightWindow.Set_is_open(true);

		ImGui::DockBuilderDockWindow(this->viewportWindow.GetWindowId().c_str(), l);
		ImGui::DockBuilderDockWindow(this->inspectorwindow.GetWindowId().c_str(), r_u);
		ImGui::DockBuilderDockWindow(this->anitobuilderWindow.GetWindowId().c_str(), r_d);
		ImGui::DockBuilderDockWindow(this->lightWindow.GetWindowId().c_str(), r_d);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	this->menubar.Draw();

	for (const auto& window : this->windows)
	{
		if (window->Get_is_open())
			window->Draw();
	}

	ImGui::Render();
}

void gbe::Editor::RenderPass()
{
	throw new std::runtime_error("Editor::RenderPass not implemented for BGFX.");
}
