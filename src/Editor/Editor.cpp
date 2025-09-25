#include "Editor.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl2.h>

#include <ImGuizmo.h>

#include <typeinfo>

#include "Graphics/gbe_graphics.h"
#include "Engine/gbe_engine.h"
#include "Physics/gbe_physics.h"

#include "Ext/GabVulkan/Objects.h"

#include "Ext/GabVulkan/Objects.h"

gbe::Editor* gbe::Editor::instance = nullptr;

gbe::Editor::Editor(RenderPipeline* renderpipeline, Window* window, Time* _mtime):
	menubar(this->windows),

	spawnWindow(this->selected),
	inspectorwindow(this->selected),
	
	gizmoLayer(this->selected)
{
	instance = this;

	this->mwindow = window;
	this->mrenderpipeline = renderpipeline;
	this->mtime = _mtime;

	//===========================IMGUI=============================//
	std::vector<VkDescriptorPoolSize> pool_sizes =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	gui_ds = new vulkan::DescriptorPool(pool_sizes, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

	TextureLoader::Set_Ui_Callback([](vulkan::Sampler* sampler, vulkan::ImageView* imgview) {
		return ImGui_ImplVulkan_AddTexture(sampler->GetData(), imgview->GetData(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

	ImGui::CreateContext(); //init self
	ImGui_ImplSDL2_InitForVulkan(static_cast<SDL_Window*>(window->Get_implemented_window())); //init for sdl
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkan::Instance::GetActive()->GetData();
	init_info.PhysicalDevice = vulkan::PhysicalDevice::GetActive()->GetData();
	init_info.Device = vulkan::VirtualDevice::GetActive()->GetData();
	init_info.Queue = vulkan::VirtualDevice::GetActive()->Get_graphicsQueue();
	init_info.DescriptorPool = gui_ds->GetData();
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.RenderPass = vulkan::RenderPass::GetActive("main")->GetData();
	ImGui_ImplVulkan_Init(&init_info); //init for vulkan


	//IO FLAGS
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

gbe::Editor::~Editor()
{
	delete gui_ds;
}

void gbe::Editor::SelectSingle(Object* other) {
	if(other->GetEditorFlag(Object::SELECT_PARENT_INSTEAD)) {
		if(other->GetParent() != nullptr)
			other = other->GetParent();
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
		if (sdlevent->button.button == SDL_BUTTON_LEFT && !this->pointer_inUi) {

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

					//CHECK IF OTHER HAS A RENDERER, IF NONE, DONT CLICK
					bool has_renderer = false;
					RenderObject* renderer_has = nullptr;

					objinray->CallRecursively([&](Object* child) {
						auto renderer_check = dynamic_cast<RenderObject*>(child);

						if (renderer_check != nullptr) {
							has_renderer = true;
							renderer_has = renderer_check;
						}
						});

					if (!has_renderer)
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
	ImGui::DockSpaceOverViewport(dockspace_id); // You can use different flags to customize behavior

	this->menubar.Draw();
	this->gizmoLayer.Draw();

	for (const auto& window : this->windows)
	{
		if (window->Get_is_open())
			window->Draw();
	}

	ImGui::Render();
}

void gbe::Editor::RenderPass(vulkan::CommandBuffer* cmd)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd->GetData());
}
