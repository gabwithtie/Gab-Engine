#include <typeinfo>

#include "Editor.h"

#include "Graphics/gbe_graphics.h"
#include "Engine/gbe_engine.h"

#include "Ext/GabVulkan/Objects.h"

gbe::Editor* gbe::Editor::instance = nullptr;

gbe::Editor::Editor(RenderPipeline* renderpipeline, Window* window, Engine* engine, Time* _mtime)
{
	instance = this;

	this->mengine = engine;
	this->mwindow = window;
	this->mrenderpipeline = renderpipeline;
	this->mtime = _mtime;

	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
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

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	if (vkCreateDescriptorPool(vulkan::VirtualDevice::GetActive()->GetData(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to init imgui");
	}

	TextureLoader::Set_Ui_Callback([](vulkan::Sampler* sampler, vulkan::ImageView* imgview) {
		return ImGui_ImplVulkan_AddTexture(sampler->GetData(), imgview->GetData(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	//this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(static_cast<SDL_Window*>(window->Get_implemented_window()));

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkan::Instance::GetActive()->GetData();
	init_info.PhysicalDevice = vulkan::PhysicalDevice::GetActive()->GetData();
	init_info.Device = vulkan::VirtualDevice::GetActive()->GetData();
	init_info.Queue = vulkan::VirtualDevice::GetActive()->Get_graphicsQueue();
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.RenderPass = vulkan::RenderPass::GetActive()->GetData();

	ImGui_ImplVulkan_Init(&init_info);

	/*
	//add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});
		*/

	//============================GIZMOS============================//
	this->gizmo_arrow_mesh = new asset::Mesh("DefaultAssets/3D/editor/arrow.obj.gbe");
	
	auto gizmoshader = new asset::Shader("DefaultAssets/Shaders/gizmo.shader.gbe");

	auto mat_r = new asset::Material("DefaultAssets/Materials/gizmo.mat.gbe");
	auto mat_g = new asset::Material("DefaultAssets/Materials/gizmo.mat.gbe");
	auto mat_b = new asset::Material("DefaultAssets/Materials/gizmo.mat.gbe");
	mat_r->setOverride("color", Vector4(1, 0, 0, 1.0f));
	mat_g->setOverride("color", Vector4(0, 1, 0, 1.0f));
	mat_b->setOverride("color", Vector4(0, 0, 1, 1.0f));
	this->gizmo_arrow_drawcall_r = this->mrenderpipeline->RegisterDrawCall(this->gizmo_arrow_mesh, mat_r);
	this->gizmo_arrow_drawcall_g = this->mrenderpipeline->RegisterDrawCall(this->gizmo_arrow_mesh, mat_g);
	this->gizmo_arrow_drawcall_b = this->mrenderpipeline->RegisterDrawCall(this->gizmo_arrow_mesh, mat_b);

	//CREATE GIZMO BOX ASSETS
	auto wireshader = new asset::Shader("DefaultAssets/Shaders/wireframe.shader.gbe");

	this->gizmo_box_mat = new asset::Material("DefaultAssets/Materials/wireframe.mat.gbe");
	this->gizmo_box_mat->setOverride("color", Vector4(1, 1, 0, 1.0f));

	//============================UI SCREENS============================//
	this->hierarchyWindow = new gbe::editor::HierarchyWindow();
	this->inspectorwindow = new gbe::editor::InspectorWindow();
	this->consoleWindow = new gbe::editor::ConsoleWindow();
	this->spawnWindow = new gbe::editor::SpawnWindow();
	this->spawnWindow->selected = &this->selected;
	this->stateWindow = new gbe::editor::StateWindow();
	this->inspectorwindow->selected = &this->selected;
	this->menubar = new gbe::editor::MenuBar();
}

void gbe::Editor::SelectSingle(Object* other) {
	auto newlyclicked = other;
	RenderObject* renderer_has = nullptr;

	newlyclicked->CallRecursively([&](Object* child) {
		auto renderer_check = dynamic_cast<RenderObject*>(child);

		if (renderer_check != nullptr) {
			renderer_has = renderer_check;
		}
		});

	//CHECK IF OTHER IS A GIZMO
	for (auto& gizmoptr : instance->gizmo_arrows)
	{
		if (newlyclicked == (*gizmoptr)) {
			instance->held_gizmo = *gizmoptr;
		}
	}

	if (instance->held_gizmo != nullptr) {
		if (instance->selected.size() == 0)
			return;

		instance->original_selected_position = instance->selected[0]->World().position.Get();
		std::cout << "Holding Gizmo" << std::endl;
	}
	else {
		bool deselection = false;

		if (instance->keyboard_shifting) {
			auto it = std::find(instance->selected.begin(), instance->selected.end(), newlyclicked);

			// DESELECT IF FOUND
			if (it != instance->selected.end()) {
				instance->selected.erase(it);

				instance->gizmo_boxes[newlyclicked]->Destroy();
				instance->gizmo_boxes.erase(newlyclicked);

				deselection = true;
			}
		}
		else { //CLEAR SELECTION IF NOT MULTISELECTING AND CLICKED SOMETHING ELSE
			instance->DeselectAll();
		}

		if (!deselection) {
			//SELECT AND BOX
			instance->selected.push_back(newlyclicked);

			if (renderer_has != nullptr)
				instance->CreateGizmoBox(renderer_has, newlyclicked);
		}

		if (instance->selected.size() == 1) {
			instance->selected_f = instance->selected[0]->World().GetForward();
			instance->selected_r = instance->selected[0]->World().GetRight();
			instance->selected_u = instance->selected[0]->World().GetUp();

			//SPAWN GIZMO
			instance->CreateGizmoArrow(instance->f_gizmo, instance->gizmo_arrow_drawcall_b, Vector3(0, 180, 0), instance->selected_f);
			instance->CreateGizmoArrow(instance->r_gizmo, instance->gizmo_arrow_drawcall_r, Vector3(0, -90, 0), instance->selected_r);
			instance->CreateGizmoArrow(instance->u_gizmo, instance->gizmo_arrow_drawcall_g, Vector3(90, 0, 0), instance->selected_u);

			instance->current_selected_position = instance->selected[0]->World().position.Get();
		}
		else {
			//DELETE GIZMO
			for (auto& gizmoptr : instance->gizmo_arrows)
			{
				if (*gizmoptr != nullptr) {
					(*gizmoptr)->Destroy();
					(*gizmoptr) = nullptr;
				}
			}
		}
	}
}

void gbe::Editor::DeselectAll() {
	instance->selected.clear();

	//CLEAR BOXES
	for (auto& gizmoptr : instance->gizmo_boxes)
	{
		gizmoptr.second->Destroy();
	}
	instance->gizmo_boxes.clear();
}

void gbe::Editor::RegisterAction(std::function<void()> action_done, std::function<void()> undo)
{
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

void gbe::Editor::CreateGizmoArrow(gbe::PhysicsObject*& out_g, DrawCall* drawcall, Vector3 rotation, Vector3 direction) {
	if (out_g == nullptr || !Object::ValidateObject(out_g)) {
		auto newGizmo = new RigidObject(true);
		newGizmo->SetParent(mengine->GetCurrentRoot());
		newGizmo->Local().scale.Set(Vector3(0.1f, 0.1f, (gizmo_offset_distance * 2.0f)));
		newGizmo->Set_is_editor();
		
		BoxCollider* FGizmo_collider = new BoxCollider();
		FGizmo_collider->SetParent(newGizmo);
		FGizmo_collider->Local().position.Set(Vector3(0, 0, 0));
		FGizmo_collider->Set_is_editor();
		RenderObject* platform_renderer = new RenderObject(drawcall);
		platform_renderer->SetParent(newGizmo);
		platform_renderer->Set_is_editor();

		out_g = newGizmo;
	}

	//SELECTED SPECIFIC THINGS
	auto rot = Quaternion::Euler(rotation);
	out_g->Local().rotation.Set(this->selected[0]->World().rotation.Get() * rot);
	out_g->TranslateWorld(direction * gizmo_offset_distance);
}

void gbe::Editor::CreateGizmoBox(gbe::RenderObject* boxed, gbe::Object* rootboxed)
{
	if (boxed->Get_DrawCall() == nullptr)
		return;

	auto newDrawcall = this->mrenderpipeline->RegisterDrawCall(boxed->Get_DrawCall()->get_mesh(), this->gizmo_box_mat);
	RenderObject* box_renderer = new RenderObject(newDrawcall);
	box_renderer->SetParent(boxed);
	box_renderer->Set_is_editor();

	gizmo_boxes.insert_or_assign(rootboxed, box_renderer);
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
			auto current_camera = this->mengine->GetActiveCamera();
			Vector3 camera_pos = current_camera->World().position.Get();
			auto mousedir = current_camera->ScreenToRay(mwindow->GetMouseDecimalPos());

			//OBJECT SELECTION
			Vector3 ray_dir = mousedir * 10000.0f;
			auto result = physics::Raycast(camera_pos, ray_dir);
			if (result.result && Object::ValidateObject(result.other)) {
				auto newlyclicked = result.other;

				//CHECK IF OTHER HAS A RENDERER, IF NONE, DONT CLICK
				bool has_renderer = false;
				RenderObject* renderer_has = nullptr;

				newlyclicked->CallRecursively([&](Object* child) {
					auto renderer_check = dynamic_cast<RenderObject*>(child);

					if (renderer_check != nullptr) {
						has_renderer = true;
						renderer_has = renderer_check;
					}
					});

				if (has_renderer) {
					SelectSingle(newlyclicked);
				}
			}
			else { //NOTHING WAS CLICKED
				if (!this->keyboard_shifting) { //NOT MULTISELECTING
					//CLEAR SELECTION IF NOT MULTISELECTING AND CLICKED NOTHING
					this->selected.clear();

					//DELETE GIZMOS
					for (auto& gizmoptr : this->gizmo_arrows)
					{
						if (*gizmoptr != nullptr) {
							(*gizmoptr)->Destroy();
							(*gizmoptr) = nullptr;
						}
					}

					//CLEAR BOXES
					for (auto& gizmoptr : this->gizmo_boxes)
					{
						gizmoptr.second->Destroy();
					}
					this->gizmo_boxes.clear();
				}
			}
		}
	}
	if (sdlevent->type == SDL_MOUSEBUTTONUP) {
		if (sdlevent->button.button == SDL_BUTTON_LEFT) {
			//COMMIT HELD GIZMO ACTION
			if (this->held_gizmo != nullptr) {
				auto newpos = instance->selected[0]->World().position.Get();
				auto oldpos = instance->original_selected_position;
				auto what_was_edited = Editor::instance->selected[0];

				Editor::RegisterAction(
					[=]() {
						what_was_edited->SetWorldPosition(newpos);
					},
					[=]() {
						what_was_edited->SetWorldPosition(oldpos);
					});

				this->held_gizmo = nullptr;
			}
		}
	}
}

void gbe::Editor::PrepareSceneChange() {
	this->DeselectAll();
	this->hierarchyWindow->root = nullptr;
}

void gbe::Editor::UpdateSelectionGui(Object* newlyclicked) {
	
}

void gbe::Editor::PrepareFrame()
{
	//imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	auto& ui_io = ImGui::GetIO();

	this->pointer_inUi = ui_io.WantCaptureMouse;
	this->keyboard_inUi = ui_io.WantCaptureKeyboard;
}

void gbe::Editor::Update()
{
	//==============================EDITOR UPDATE==============================//
	if (selected.size() == 1) {
		auto current_camera = this->mengine->GetActiveCamera();
		Vector3 camera_pos = current_camera->World().position.Get();
		auto mousedir = current_camera->ScreenToRay(mwindow->GetMouseDecimalPos());

		if (held_gizmo != nullptr) {
			//FIND POSITION TO TRANSFORM THE SELECTED OBJECT
			//FIND DIRECTION
			Vector3 transformDirection;
			if (held_gizmo == f_gizmo)
				transformDirection = this->selected_f;
			if (held_gizmo == r_gizmo)
				transformDirection = this->selected_r;
			if (held_gizmo == u_gizmo)
				transformDirection = this->selected_u;

			// Find the closest point on the line segments
			this->current_selected_position = Vector3::GetClosestPointOnLineGivenLine(original_selected_position, transformDirection, camera_pos, mousedir);

			selected[0]->SetWorldPosition(this->current_selected_position);
		}

		Vector3 cam_toselected = this->current_selected_position - camera_pos;
		cam_toselected = cam_toselected.Normalize();

		Vector3 finalgizmopos = camera_pos + (cam_toselected * this->gizmo_fixed_depth);

		//UPDATE THE POSITION OF ALL GIZMOS
		this->f_gizmo->Local().position.Set(finalgizmopos + (gizmo_offset_distance * this->selected_f));
		this->r_gizmo->Local().position.Set(finalgizmopos + (gizmo_offset_distance * this->selected_r));
		this->u_gizmo->Local().position.Set(finalgizmopos + (gizmo_offset_distance * this->selected_u));
	}

	//==============================IMGUI==============================//
	if (this->hierarchyWindow->root == nullptr)
		this->hierarchyWindow->root = mengine->GetCurrentRoot();
	
	this->inspectorwindow->Draw();
	this->spawnWindow->Draw();
	this->stateWindow->Draw();
	this->consoleWindow->Draw();
	this->hierarchyWindow->Draw();
	this->menubar->Draw();
}

void gbe::Editor::PresentFrame()
{
	//BEFORE YOU DRAW THE EDITOR, IMPLEMENT THE SCREEN RECORDING
	

	//AFTER RECORDING, DO THE RENDER
	ImGui::Render();
}

void gbe::Editor::RenderPass(vulkan::CommandBuffer* cmd)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd->GetData());
}
