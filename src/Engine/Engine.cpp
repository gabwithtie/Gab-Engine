#include "Engine.h"

#include "Editor/gbe_editor.h"
#include "Math/gbe_math.h"
#include "Physics/gbe_physics.h"
#include "Audio/gbe_audio.h"
#include "Asset/gbe_asset.h"
#include "Extension/Extension.h"

#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>

namespace gbe {
	Engine* Engine::instance;

	Engine::Engine(std::vector<Extension*> _engine_extensions) :
		window(Vector2Int(1280, 720)),
		renderpipeline(this->window, this->window.Get_dimentions())
	{
		std::cout << "[ENGINE] Initializing..." << std::endl;

		instance = this;

		this->engine_extensions = _engine_extensions;
		std::vector<editor::GuiWindow*> extension_windows;

		for (const auto& extension : this->engine_extensions)
		{
			extension->OnEngineInitialize();

			for (const auto& extension_window : extension->extension_windows)
			{
				extension_windows.push_back(extension_window);
			}
		}

		//EDITOR SETUP
		NFD_Init();

		editor = new Editor(&renderpipeline, &window, &time, extension_windows);
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

		for (const auto& cpair : camera_handler->object_list)
		{
			auto cam = cpair.second;

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

			auto& objlist = instance->current_root->GetHandler<PhysicsObject>()->object_list;
			for (const auto& pair : objlist) {
				auto& physicsobject = pair.second;
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

		//Wait here for all async tasks to finish
		bool batchload_done = false;
		while (!batchload_done)
		{
			batchload_done = true;

			for (const auto& loader : gbe::asset::all_asset_loaders)
			{
				if (loader->CheckAsynchrounousTasks() > 0) {
					batchload_done = false;
				}
			}
		}

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
				for (auto& pair : inputhandler->object_list) {
					auto& input_player = pair.second;

					if (!input_player->Get_enabled())
						continue;
					if (input_player->get_player_name() != name)
						continue;
					if (this->Get_state() == EngineState::Edit && !input_player->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
						continue;
					if (this->Get_state() != EngineState::Edit && input_player->GetEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE))
						continue;

					for (auto& pair : input_player->controllers.object_list) {
						auto& controller = pair.second;

						controller->ForEach_inputreceivers([action, changed](InputCustomer_base* input_customer) {
							input_customer->TryReceive(action, changed);
							});
					}
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
			frameinfo.pointer_pixelpos = this->window.GetMousePixelPos();

			//Lights colating
			this->current_root->GetHandler<LightObject>()->DoOnEnabled([&](LightObject* light) {
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
				}, false);

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