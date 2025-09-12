#include "Engine.h"

#include "Ext/AnitoBuilderWrapper/AnitoBuilderWrapper.h"
#include "Ext/AnitoBuilderWrapper/BuilderBlock.h"
#include "Ext/AnitoBuilder/AnitoBuilder.h"

#include "Editor/gbe_editor.h"
#include "GUI/gbe_gui.h"
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
			if (instance->Get_state() == EngineState::Edit && !cam->Get_is_editor())
				continue;
			if (instance->Get_state() != EngineState::Edit && cam->Get_is_editor())
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

	void Engine::Set_state(Engine::EngineState _state) {
		if (_state == EngineState::Edit) {
			if (instance->pre_play_scenedata == nullptr)
				instance->pre_play_scenedata = new SerializedObject(instance->current_root->Serialize());

			auto newroot = gbe::Engine::CreateBlankRoot();
			newroot->Deserialize(*instance->pre_play_scenedata);

			gbe::Engine::ChangeRoot(newroot);

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
		//MESH CACHING
		auto cube_mesh = new asset::Mesh("DefaultAssets/3D/default/cube.obj.gbe");
		auto sphere_mesh = new asset::Mesh("DefaultAssets/3D/default/sphere.obj.gbe");
		auto capsule_mesh = new asset::Mesh("DefaultAssets/3D/default/capsule.obj.gbe");
		auto plane_mesh = new asset::Mesh("DefaultAssets/3D/default/plane.obj.gbe");
		//MESH CACHING
		//SHADER CACHING
		auto gridShader = new asset::Shader("DefaultAssets/Shaders/grid.shader.gbe");
		//TEXTURE CACHING
		//MATERIAL CACHING
		auto grid_mat = new asset::Material("DefaultAssets/Materials/grid.mat.gbe");

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
		Engine::MakePersistent(editor_input);
		editor_input->Set_is_editor();

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
		Vector3 cubecorners[4] = {
			Vector3(-2, 0, -2),
			Vector3(2, 0, -2),
			Vector3(2, 0, 2),
			Vector3(-2, 0, 2),
		};
		auto builder_cube = new ext::AnitoBuilder::BuilderBlock(cubecorners, 4);
		builder_cube->SetParent(this->current_root);
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

			mInputSystem->UpdateStates([=](std::string name, gbe::input::InputAction* action, bool changed) {
				for (auto input_player : inputhandler->t_object_list) {
					if (!input_player->Get_enabled())
						continue;
					if (input_player->get_player_name() != name)
						continue;
					if (this->Get_state() == EngineState::Edit && !input_player->Get_is_editor())
						continue;
					if (this->Get_state() != EngineState::Edit && input_player->Get_is_editor())
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

			//Update Render pipeline
			//EDITOR UPDATE
			editor->PrepareFrame();
			editor->Update();
			//<----------MORE EDITOR FUNCTIONS GO HERE
			editor->PresentFrame();
			//ENGINE UPDATE
			this->current_root->GetHandler<LightObject>()->DoOnEnabled([](LightObject* light) {
				RenderPipeline::Get_Instance()->TryPushLight(light->GetData(), false);
				});

			auto pos = Vector3::zero;
			auto forward = Vector3::zero;
			auto frustrum = Matrix4();
			auto viewm = Matrix4();
			auto projm = Matrix4();
			auto nearclip = 0.0f;
			auto farclip = 0.0f;

			Camera* current_camera = GetActiveCamera();
			if (current_camera != nullptr) {
				pos = current_camera->World().position.Get();
				forward = current_camera->World().GetForward();
				frustrum = current_camera->GetProjectionMat() * current_camera->GetViewMat();
				viewm = current_camera->GetViewMat();
				projm = current_camera->GetProjectionMat();
				nearclip = current_camera->nearClip;
				farclip = current_camera->farClip;
			}
			else {
				throw std::runtime_error("No cameras rendering scene.");
			}
			renderpipeline.RenderFrame(viewm, projm, nearclip, farclip);
			//mGUIPipeline->DrawActiveCanvas();

			//Update the window
			this->window.SwapBuffers();

			//Update other handlers
			auto physicshandler_generic = this->current_root->GetHandler<PhysicsObject>();
			auto physicshandler = static_cast<PhysicsHandler*>(physicshandler_generic);
			auto updatehandler = this->current_root->GetHandler<Update>();
			auto lateupdatehandler = this->current_root->GetHandler<LateUpdate>();

			auto onTick = [=](double deltatime) {
				physicshandler->Update(deltatime);

				float delta_f = (float)deltatime;

				//Normal Update
				updatehandler->DoOnEnabled([delta_f](Update* updatable) {
					updatable->InvokeUpdate(delta_f);
					});
				//Late Update
				lateupdatehandler->DoOnEnabled([delta_f](LateUpdate* updatable) {
					updatable->InvokeLateUpdate(delta_f);
					});

				instance->timeleft_stepping -= deltatime;
				};
			this->time.TickFixed(onTick);

			mInputSystem->ResetStates(&this->window);

			//Queued root change
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

			//Delete all queued for deletions
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