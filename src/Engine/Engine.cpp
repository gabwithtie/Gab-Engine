#include "Engine.h"

#include "Ext/AnitoBuilderWrapper/AnitoBuilderWrapper.h"
#include "Ext/AnitoBuilderWrapper/BuilderBlock.h"
#include "Ext/AnitoBuilder/AnitoBuilder.h"

#include "Editor/gbe_editor.h"
#include "Math/gbe_math.h"
#include "Physics/gbe_physics.h"
#include "Audio/gbe_audio.h"
#include "Asset/gbe_asset.h"

namespace gbe {
	Engine* Engine::instance;

	Engine::Engine() : 
		window(Vector2Int(1280, 720)),
		renderpipeline(this->window, this->window.Get_dimentions())
	{
		instance = this;

		//EDITOR SETUP
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

	Root* Engine::CreateBlankRoot()
	{
		auto root_object = new Root();
		root_object->RegisterHandler(new PhysicsHandler());
		root_object->RegisterHandler(new ObjectHandler<gbe::LightObject>());
		root_object->RegisterHandler(new ObjectHandler<gbe::Camera>());
		root_object->RegisterHandler(new ObjectHandler<PhysicsUpdate>());
		root_object->RegisterHandler(new ObjectHandler<InputPlayer>());
		root_object->RegisterHandler(new ObjectHandler<EarlyUpdate>());
		root_object->RegisterHandler(new ObjectHandler<Update>());
		root_object->RegisterHandler(new ObjectHandler<LateUpdate>());

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
					instance->current_root = instance->CreateBlankRoot();
				if (instance->pre_play_scenedata == nullptr)
					instance->pre_play_scenedata = new SerializedObject(instance->current_root->Serialize());

				auto newroot = gbe::Engine::CreateBlankRoot();
				newroot->Deserialize(*instance->pre_play_scenedata);

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

		//MESH FINDING
		auto cube_mesh = asset::Mesh::GetAssetById("cube");
		auto sphere_mesh = asset::Mesh::GetAssetById("sphere");
		auto capsule_mesh = asset::Mesh::GetAssetById("capsule");
		auto plane_mesh = asset::Mesh::GetAssetById("plane");
		//MATERIAL FINDING
		auto grid_mat = asset::Material::GetAssetById("grid");
		auto lit_mat = asset::Material::GetAssetById("lit");

		//DRAW CALL CACHING X PRIMITIVES CACHING
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::cube, renderpipeline.RegisterDrawCall(cube_mesh, grid_mat));
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::sphere, renderpipeline.RegisterDrawCall(sphere_mesh, grid_mat));
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::plane, renderpipeline.RegisterDrawCall(plane_mesh, grid_mat));
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::capsule, renderpipeline.RegisterDrawCall(capsule_mesh, grid_mat));

		//TYPE SERIALIZER REGISTERING
		gbe::TypeSerializer::RegisterTypeCreator(typeid(RenderObject).name(), RenderObject::Create);
		gbe::TypeSerializer::RegisterTypeCreator(typeid(RigidObject).name(), RigidObject::Create);
		gbe::TypeSerializer::RegisterTypeCreator(typeid(BoxCollider).name(), BoxCollider::Create);
		gbe::TypeSerializer::RegisterTypeCreator(typeid(SphereCollider).name(), SphereCollider::Create);
		gbe::TypeSerializer::RegisterTypeCreator(typeid(CapsuleCollider).name(), CapsuleCollider::Create);
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
#pragma region Root Loaders
		this->current_root = this->CreateBlankRoot();
		this->InitializeRoot();
		this->Set_state(EngineState::Edit, false);
#pragma region scene singletons
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
			platform_renderer->SetShadowCaster();

			return parent;
			};

		//PERSISTENT OBJECTS
		//GRAVITY
		auto gravity_volume = new ForceVolume();
		gravity_volume->SetParent(this->current_root);
		Engine::MakePersistent(gravity_volume);

		//EDITOR CAMERA
		auto editor_input = new InputPlayer(player_name);
		editor_input->SetParent(this->current_root);
		auto editor_camera_controller = new FlyingCameraControl();
		editor_camera_controller->SetParent(editor_input);
		PerspectiveCamera* editor_cam = new PerspectiveCamera(&this->window);
		editor_cam->SetParent(editor_camera_controller);
		editor_cam->farClip = 200;
		Engine::MakePersistent(editor_input);
		editor_cam->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);
		editor_input->PushEditorFlag(Object::EXCLUDE_FROM_OBJECT_TREE);

		//PLAYER CAMERA
		auto player_input = new InputPlayer(player_name);
		player_input->SetParent(this->current_root);
		auto camera_controller = new FlyingCameraControl();
		camera_controller->SetParent(player_input);
		PerspectiveCamera* player_cam = new PerspectiveCamera(&this->window);
		player_cam->SetParent(camera_controller);
		Engine::MakePersistent(player_input);
#pragma endregion

#pragma region scene objects
		//GRID
		const Vector2Int GridSize(10, 10);

		for (size_t i = 0; i < GridSize.x * GridSize.y; i++)
		{
			
		}

		//Frustrum debugging
		auto test_sphere = create_primitive(gbe::RenderObject::sphere, Vector3(0, 2, -5), Vector3(1));


		//ANITO BUILDER

		Vector3 cubecorners[4] = {
			Vector3(-2, 0, -2),
			Vector3(2, 0, -2),
			Vector3(2, 0, 2),
			Vector3(-2, 0, 2),
		};
		auto builder_cube = new ext::AnitoBuilder::BuilderBlock(cubecorners, 4);
		builder_cube->SetParent(this->current_root);

		auto dirlight = new DirectionalLight();
		dirlight->World().position.Set(Vector3(0, 0, -10));
		dirlight->SetName("Directional Light");
		dirlight->SetParent(this->current_root);

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
					renderpipeline.SetResolution(newdimensions);
				}
			}

			//Update input system
			auto inputhandler = this->current_root->GetHandler<InputPlayer>();

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

			RenderPipeline::FrameRenderInfo frameinfo{};

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
				frameinfo.projmat_lightusage = current_camera->GetProjectionMat(20.0f);
			}
			else {
				throw std::runtime_error("No cameras rendering scene.");
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