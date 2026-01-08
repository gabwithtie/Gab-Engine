#include "Engine.h"

#include "Ext/AnitoBuilderWrapper/BuilderBlock.h"

#include "Editor/gbe_editor.h"
#include "Math/gbe_math.h"
#include "Physics/gbe_physics.h"
#include "Audio/gbe_audio.h"
#include "Asset/gbe_asset.h"

#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>

namespace gbe {
	Engine* Engine::instance;

	Engine::Engine() : 
		window(Vector2Int(1280, 720)),
		renderpipeline(this->window, this->window.Get_dimentions())
	{
		std::cout << "[ENGINE] Initializing..." << std::endl;

		instance = this;

		//EDITOR SETUP
		NFD_Init();

		editor = new Editor(&renderpipeline, &window, &time);
		if (editor != nullptr) {
			this->window.AddAdditionalEventProcessor([=](void* newevent) {
				editor->ProcessRawWindowEvent(newevent);
				});
		}
	}

	bool Engine::ChangeRoot(Root* newroot)
	{
		instance->queued_rootchange = newroot;
		return true;
	}

	Root* Engine::CreateBlankRoot(SerializedObject* data)
	{
		Root* root_object = nullptr;

		if (data == nullptr)
			root_object = new Root();
		else
			root_object = new Root(data);

		root_object->RegisterHandler(new PhysicsHandler());
		root_object->RegisterHandler(new ObjectHandler<gbe::LightObject>());
		root_object->RegisterHandler(new ObjectHandler<gbe::Camera>());
		root_object->RegisterHandler(new ObjectHandler<PhysicsUpdate>());
		root_object->RegisterHandler(new ObjectHandler<InputPlayer>());
		root_object->RegisterHandler(new ObjectHandler<EarlyUpdate>());
		root_object->RegisterHandler(new ObjectHandler<Update>());
		root_object->RegisterHandler(new ObjectHandler<LateUpdate>());

		if (data != nullptr)
			root_object->LoadChildren(data);

		return root_object;
	}

	Camera* Engine::GetActiveCamera() {
		Camera* current_camera = nullptr;
		auto camera_handler = instance->current_root->GetHandler<Camera>();

		for (const auto cam : camera_handler->t_object_list)
		{
			if (!cam->Get_enabled())
				continue;
			if (instance->Get_state() == EngineState::Edit && !cam->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
				continue;
			if (instance->Get_state() != EngineState::Edit && cam->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
				continue;

			current_camera = cam;
			break;
		}

		return current_camera;
	}

	void Engine::InitializeRoot() {
		for (const auto persistent : this->persistents)
		{
			persistent->SetParent(this->current_root);
		}

		//Push new context for physics singletons
		auto physicshandler_generic = this->current_root->GetHandler<PhysicsObject>();
		auto physicshandler = static_cast<PhysicsHandler*>(physicshandler_generic);
		physics::PhysicsPipeline::PushContext(physicshandler->GetLocalPipeline());
	}

	void Engine::Set_state(Engine::EngineState _state, bool change_scene) {
		if (_state == EngineState::Edit) {
			if (change_scene) {
				if (instance->current_root == nullptr)
					instance->current_root = instance->CreateBlankRoot(nullptr);
				if (instance->pre_play_scenedata == nullptr)
					instance->pre_play_scenedata = new SerializedObject(instance->current_root->Serialize());

				auto newroot = gbe::Engine::CreateBlankRoot(instance->pre_play_scenedata);

				gbe::Engine::ChangeRoot(newroot);
			}

			instance->time.scale = 0;
			Console::Log("Entering Edit Mode...");
		}
		if (_state == EngineState::Play) {
			if(instance->state == EngineState::Edit)
				instance->pre_play_scenedata = new SerializedObject(instance->current_root->Serialize());

			instance->time.scale = 1;

			auto objlist = instance->current_root->GetHandler<PhysicsObject>()->t_object_list;
			for (const auto physicsobject : objlist) {
				if (!physicsobject->Get_enabled())
					continue;
				
				physicsobject->ForceWake();
			}

			if (instance->state == EngineState::Edit)
				Console::Log("Entering Play Mode...");

		}
		if (_state == EngineState::Paused) {
			instance->time.scale = 0;
		}

		instance->state = _state;
	}

	void Engine::Step(double dur) {
		instance->timeleft_stepping = dur;
	}

	void Engine::Run()
	{
#pragma region Asset Loading
		asset::BatchLoader::LoadAssetsFromDirectory("DefaultAssets");
		asset::BatchLoader::LoadAssetsFromDirectory("cache");

		//Init all that needs assets here
		renderpipeline.InitializeAssetRequisites();

		//MESH FINDING
		auto cube_mesh = asset::Mesh::GetAssetById("cube");
		auto sphere_mesh = asset::Mesh::GetAssetById("sphere");
		auto capsule_mesh = asset::Mesh::GetAssetById("capsule");
		auto plane_mesh = asset::Mesh::GetAssetById("plane");
		//MATERIAL FINDING
		auto grid_mat = asset::Material::GetAssetById("grid");
		auto wire_mat = asset::Material::GetAssetById("wireframe");
		auto lit_mat = asset::Material::GetAssetById("lit");

		//DRAW CALL CACHING X PRIMITIVES CACHING
		{
			auto newcall = renderpipeline.RegisterDrawCall(cube_mesh, lit_mat);
			RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::cube, newcall);
		}
		{
			auto newcall = renderpipeline.RegisterDrawCall(sphere_mesh, lit_mat);
			RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::sphere, newcall);
		}
		{
			auto newcall = renderpipeline.RegisterDrawCall(plane_mesh, lit_mat);
			RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::plane, newcall);
		}
		{
			auto newcall = renderpipeline.RegisterDrawCall(capsule_mesh, lit_mat);
			RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::capsule, newcall);
		}

		
		
		//TYPE SERIALIZER REGISTERING
		gbe::TypeSerializer::RegisterTypeCreator(typeid(RenderObject).name(), [](SerializedObject* data) {return new RenderObject(data); });

		gbe::TypeSerializer::RegisterTypeCreator(typeid(DirectionalLight).name(), [](SerializedObject* data) {return new DirectionalLight(data); });
		gbe::TypeSerializer::RegisterTypeCreator(typeid(ConeLight).name(), [](SerializedObject* data) {return new ConeLight(data); });
		
		gbe::TypeSerializer::RegisterTypeCreator(typeid(ext::AnitoBuilder::BuilderBlock).name(), [](SerializedObject* data) {return new ext::AnitoBuilder::BuilderBlock(data); });

#pragma endregion
#pragma region Input
		auto mInputSystem = new InputSystem();
		auto player_name = "MAIN";
		mInputSystem->RegisterActionListener(player_name, new KeyPressImplementation<Keys::MOUSE_LEFT>());
		mInputSystem->RegisterActionListener(player_name, new WasdDeltaImplementation());
		mInputSystem->RegisterActionListener(player_name, new MouseScrollImplementation());
		mInputSystem->RegisterActionListener(player_name, new MouseDeltaImplementation());
		mInputSystem->RegisterActionListener(player_name, new KeyPressImplementation<Keys::SPACE>());
		mInputSystem->RegisterActionListener(player_name, new KeyPressImplementation<Keys::ESCAPE>());
		mInputSystem->RegisterActionListener(player_name, new MouseDragImplementation<Keys::MOUSE_RIGHT>());
		mInputSystem->RegisterActionListener(player_name, new MouseDragImplementation<Keys::MOUSE_MIDDLE>());
#pragma endregion
#pragma region scene helpers
		//Spawn funcs
		auto create_mesh = [&](gfx::DrawCall* drawcall, Vector3 pos, Vector3 scale, Quaternion rotation = Quaternion::Euler(Vector3(0, 0, 0))) {
			RigidObject* parent = new RigidObject(true);
			parent->SetParent(this->current_root);
			parent->Local().position.Set(pos);
			parent->Local().rotation.Set(rotation);
			parent->Local().scale.Set(scale);
			MeshCollider* meshcollider = new MeshCollider(drawcall->get_mesh());
			meshcollider->SetParent(parent);
			meshcollider->Local().position.Set(Vector3(0, 0, 0));
			RenderObject* platform_renderer = new RenderObject(drawcall);
			platform_renderer->SetParent(parent);

			return parent;
			};

		auto create_primitive = [&](RenderObject::PrimitiveType ptype, Vector3 pos, Vector3 scale, Quaternion rotation = Quaternion::Euler(Vector3(0, 0, 0)), bool _static = true) {
			RigidObject* parent = new RigidObject(_static);
			parent->SetParent(this->current_root);
			parent->Local().position.Set(pos);
			parent->Local().rotation.Set(rotation);
			parent->Local().scale.Set(scale);
			BoxCollider* meshcollider = new BoxCollider();
			meshcollider->SetParent(parent);
			meshcollider->Local().position.Set(Vector3(0, 0, 0));
			RenderObject* platform_renderer = new RenderObject(ptype);
			platform_renderer->SetParent(parent);

			return parent;
			};
#pragma endregion
#pragma region Root Loaders
		SerializedObject savedscene;
		if (gbe::asset::serialization::gbeParser::PopulateClass(savedscene, "out/default.level")) {
			this->current_root = this->CreateBlankRoot(&savedscene);
		}
		else {
			this->current_root = this->CreateBlankRoot();

			create_primitive(RenderObject::cube, Vector3(0, 0, 0), Vector3(2));
			create_primitive(RenderObject::cube, Vector3(0, 0, 5), Vector3(2));
			create_primitive(RenderObject::cube, Vector3(0, 0, 10), Vector3(2));
			create_primitive(RenderObject::cube, Vector3(0, 0, 15), Vector3(2));
			create_primitive(RenderObject::cube, Vector3(0, 0, 20), Vector3(2));
			create_primitive(RenderObject::cube, Vector3(0, 0, 25), Vector3(2));
		}
		this->InitializeRoot();
		this->Set_state(EngineState::Edit, false);
#pragma region scene singletons
		//PERSISTENT OBJECTS

		//EDITOR CAMERA
		auto editor_input = new InputPlayer(player_name);
		editor_input->SetParent(this->current_root);
		auto editor_camera_controller = new FlyingCameraControl();
		editor_camera_controller->SetParent(editor_input);
		PerspectiveCamera* editor_cam = new PerspectiveCamera();
		editor_cam->SetParent(editor_camera_controller);
		editor_cam->farClip = 200;
		Engine::MakePersistent(editor_input);
		editor_cam->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);
		editor_input->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);
		editor_camera_controller->World().position.Set(Vector3(0, 3, -15));
#pragma endregion

#pragma region scene objects
#pragma endregion

#pragma endregion
#pragma region MAIN LOOP

		/// MAIN GAME LOOP
		while (!this->window.ShouldClose())
		{
			if (this->state == Engine::EngineState::Paused) {
				if (this->timeleft_stepping > 0)
					this->time.scale = 1;
				else
					this->time.scale = 0;
			}

			/* Poll for and process events */
			this->window.UpdateState();
			gbe::window::WindowEventType windoweventtype;
			while (this->window.PollEvents(windoweventtype))
			{
				if (windoweventtype == gbe::window::WindowEventType::RESIZE) {
					auto newdimensions = this->window.Get_dimentions();
					renderpipeline.SetScreenResolution(newdimensions);
				}
			}

			//Update input system
			auto inputhandler = this->current_root->GetHandler<InputPlayer>();


			auto teststate = this->window.GetKeyState(1) == true;
			if (editor != nullptr && !editor->FocusedOnEditorUI())
				mInputSystem->UpdateStates([=](std::string name, gbe::input::InputAction* action, bool changed) {
				for (auto input_player : inputhandler->t_object_list) {
					if (!input_player->Get_enabled())
						continue;
					if (input_player->get_player_name() != name)
						continue;
					if (this->Get_state() == EngineState::Edit && !input_player->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
						continue;
					if (this->Get_state() != EngineState::Edit && input_player->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
						continue;

					for (auto controller : input_player->controllers.t_object_list)
						controller->ForEach_inputreceivers([action, changed](InputCustomer_base* input_customer) {
						input_customer->TryReceive(action, changed);
							});
				}
					}, &this->window);

			//Update GUI system

			//Early update
			this->current_root->GetHandler<EarlyUpdate>()->DoOnEnabled([](EarlyUpdate* updatable) {
				updatable->InvokeEarlyUpdate();
				});

			//=======================RENDERING======================//
			//EDITOR pushing
			editor->PrepareUpdate();

			gfx::SceneRenderInfo frameinfo{};

			//Lights colating
			this->current_root->GetHandler<LightObject>()->DoOnEnabled([&](LightObject* light) {
				PointLight* plight = nullptr;
				plight = dynamic_cast<PointLight*>(light);

				if (plight != nullptr)
					plight->Resync_sublights();
				else
					frameinfo.lightdatas.push_back(light->GetData());
			});

			Camera* current_camera = GetActiveCamera();
			if (current_camera != nullptr) {
				frameinfo.camera_pos = current_camera->World().position.Get();
				frameinfo.farclip = current_camera->farClip;
				frameinfo.nearclip = current_camera->nearClip;
				frameinfo.viewmat = current_camera->GetViewMat();
				frameinfo.projmat = current_camera->GetProjectionMat();
				frameinfo.projmat_lightusage = current_camera->GetProjectionMat(20);
				
				if (current_camera->GetMoved()) {
					frameinfo.skip_main_pass = false;
					current_camera->OnRender();
				}
				else
					frameinfo.skip_main_pass = true;

				//GRID
				const int gridlines = 100;
				const float stride = 2 * ceil(abs(frameinfo.camera_pos.y) / 10.0f);
				const int line_length = gridlines * stride;
				const int half_bounds = line_length / 2;
				const auto align = [stride](float coord) {
					coord /= stride;
					coord = round(coord);
					coord *= stride;
					return coord;
					};

				float max_z = align(frameinfo.camera_pos.z + half_bounds);
				float max_x = align(frameinfo.camera_pos.x + half_bounds);
				float min_z = align(frameinfo.camera_pos.z - half_bounds);
				float min_x = align(frameinfo.camera_pos.x - half_bounds);

				const auto propagatelines = [=](float coef) {
					Vector3 start_cor = Vector3(align(frameinfo.camera_pos.x), 0, align(frameinfo.camera_pos.z));
					start_cor.y = 0;
					Vector3 cur_x = start_cor;
					Vector3 cur_z = start_cor;

					for (size_t l_i = 0; l_i < gridlines / 2; l_i++)
					{
						Vector3 x_a = cur_x - Vector3(0, 0, half_bounds);
						Vector3 x_b = cur_x + Vector3(0, 0, half_bounds);
						//RenderPipeline::DrawLine(x_a, x_b);

						Vector3 z_a = cur_z - Vector3(half_bounds, 0, 0);
						Vector3 z_b = cur_z + Vector3(half_bounds, 0, 0);
						//RenderPipeline::DrawLine(z_a, z_b);

						auto dist = abs(frameinfo.camera_pos.z - cur_z.z);
						auto skip_coef = abs(1.0f / (frameinfo.camera_pos.y / 16.0f)) * (dist / 90.0f);
						auto stride_scale = 1 + floor(skip_coef);
						auto final_step = stride_scale * stride;

						cur_x += Vector3(final_step, 0, 0) * coef;
						cur_z += Vector3(0, 0, final_step) * coef;
					}
				};

				propagatelines(1);
				propagatelines(-1);
			}
			else {

			}

			renderpipeline.RenderFrame(frameinfo);

			this->window.SwapBuffers();

			//======================Physics and Normal/Late Update=======================//
			auto physicshandler_generic = this->current_root->GetHandler<PhysicsObject>();
			auto physicshandler = static_cast<PhysicsHandler*>(physicshandler_generic);
			auto updatehandler = this->current_root->GetHandler<Update>();
			auto lateupdatehandler = this->current_root->GetHandler<LateUpdate>();

			auto onTick = [=](double deltatime) {
				physicshandler->Update(deltatime);
				};

			this->time.UpdateTime();
			this->time.TickFixed(onTick);

			//Normal Update
			updatehandler->DoOnEnabled([this](Update* updatable) {
				updatable->InvokeUpdate(this->time.GetDeltaTime());
				});
			//Late Update
			lateupdatehandler->DoOnEnabled([this](LateUpdate* updatable) {
				updatable->InvokeLateUpdate(this->time.GetDeltaTime());
				});
			instance->timeleft_stepping -= this->time.GetDeltaTime();

			mInputSystem->ResetStates(&this->window);

			//=======================DELETION======================//
			if (this->queued_rootchange != nullptr) {
				editor->PrepareSceneChange();

				for (const auto persistent : this->persistents)
				{
					persistent->SetParent(nullptr);
				}

				if (this->current_root != nullptr)
				{
					this->current_root->Destroy();
				}
			}

			std::list<Object*> toDeleteRoots;

			this->current_root->CallRecursively([&toDeleteRoots](Object* object) {
				if (object->get_isDestroyed()) {
					toDeleteRoots.push_back(object);
				}
				});

			for (auto rootdeletee : toDeleteRoots)
			{
				rootdeletee->SetParent(nullptr);
			}

			for (auto rootdeletee : toDeleteRoots)
			{
				delete rootdeletee;
			}

			if (this->queued_rootchange != nullptr) {
				this->current_root = this->queued_rootchange;
				this->queued_rootchange = nullptr;

				this->InitializeRoot();
			}
		}
#pragma endregion
	}
}