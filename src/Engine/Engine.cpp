#include "Engine.h"
#include "gbe_engine.h"

#include "Editor/gbe_editor.h"
#include "Graphics/gbe_graphics.h"
#include "GUI/gbe_gui.h"
#include "Math/gbe_math.h"
#include "Physics/gbe_physics.h"
#include "Window/gbe_window.h"
#include "Audio/gbe_audio.h"
#include "Asset/gbe_asset.h"

namespace gbe {
	Engine* Engine::instance;

	Engine::Engine()
	{
		instance = this;
		this->current_root = nullptr;
		this->queued_rootchange = nullptr;
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

			instance->_time.scale = 0;
			Console::Log("Entering Edit Mode...");
		}
		if (_state == EngineState::Play) {
			if(instance->state == EngineState::Edit)
				instance->pre_play_scenedata = new SerializedObject(instance->current_root->Serialize());

			instance->_time.scale = 1;

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
			instance->_time.scale = 0;
		}

		instance->state = _state;
	}

	void Engine::Step(double dur) {
		instance->timeleft_stepping = dur;
	}

	void Engine::Run()
	{
		//WINDOW
		Window* mWindow = new Window(Vector2Int(1280, 720));

#pragma region Rendering Pipeline Setup
		//RenderPipeline setup
		auto mRenderPipeline = new RenderPipeline(mWindow, mWindow->Get_dimentions());
#pragma endregion
		//GLOBAL RUNTIME COMPONENTS
		this->_time = Time();
#pragma region Audio Pipeline Setup
		auto mAudioPipeline = new audio::AudioPipeline();
		//mAudioPipeline->Init();
#pragma endregion
#pragma region Editor Setup
		auto mEditor = new gbe::Editor(mRenderPipeline, mWindow, this, &this->_time);
		mWindow->AddAdditionalEventProcessor([mEditor](void* newevent) {
			mEditor->ProcessRawWindowEvent(newevent);
			});
#pragma endregion
#pragma region Asset Loading
		//AUDIO CACHING

		//MESH CACHING
		auto cube_mesh = new asset::Mesh("DefaultAssets/3D/default/cube.obj.gbe");
		auto sphere_mesh = new asset::Mesh("DefaultAssets/3D/default/sphere.obj.gbe");
		auto capsule_mesh = new asset::Mesh("DefaultAssets/3D/default/capsule.obj.gbe");
		auto plane_mesh = new asset::Mesh("DefaultAssets/3D/default/plane.obj.gbe");

		//SHADER CACHING
		auto unlitShader = new asset::Shader("DefaultAssets/Shaders/unlit.shader.gbe");
		auto gridShader = new asset::Shader("DefaultAssets/Shaders/grid.shader.gbe");
		auto wireShader = new asset::Shader("DefaultAssets/Shaders/wireframe.shader.gbe");

		//TEXTURE CACHING
		auto test_tex = new asset::Texture("DefaultAssets/Tex/Maps/Model/test.img.gbe");
		auto logo_tex = new asset::Texture("DefaultAssets/Tex/UI/logo.img.gbe");

		//MATERIAL CACHING
		auto grid_mat = new asset::Material("DefaultAssets/Materials/grid.mat.gbe");
		grid_mat->setOverride("color", Vector4(0.3, 1, 0.3, 1.0f));
		grid_mat->setOverride("colortex", test_tex);
		auto cube_mat = new asset::Material("DefaultAssets/Materials/grid.mat.gbe");
		cube_mat->setOverride("colortex", test_tex);
		auto wire_mat = new asset::Material("DefaultAssets/Materials/wireframe.mat.gbe");

		//DRAW CALL CACHING X PRIMITIVES CACHING
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::cube, mRenderPipeline->RegisterDrawCall(cube_mesh, cube_mat));
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::sphere, mRenderPipeline->RegisterDrawCall(sphere_mesh, cube_mat));
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::plane, mRenderPipeline->RegisterDrawCall(plane_mesh, cube_mat));
		RenderObject::RegisterPrimitiveDrawcall(RenderObject::PrimitiveType::capsule, mRenderPipeline->RegisterDrawCall(capsule_mesh, cube_mat));

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

		auto create_primitive = [&](RenderObject::PrimitiveType ptype, Vector3 pos, Vector3 scale, Quaternion rotation = Quaternion::Euler(Vector3(0, 0, 0))) {
			RigidObject* parent = new RigidObject(true);
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
		PerspectiveCamera* editor_cam = new PerspectiveCamera(mWindow);
		editor_cam->SetParent(editor_camera_controller);
		Engine::MakePersistent(editor_input);
		editor_input->Set_is_editor();

		//PLAYER CAMERA
		auto player_input = new InputPlayer(player_name);
		player_input->SetParent(this->current_root);
		auto camera_controller = new FlyingCameraControl();
		camera_controller->SetParent(player_input);
		PerspectiveCamera* player_cam = new PerspectiveCamera(mWindow);
		player_cam->SetParent(camera_controller);
		Engine::MakePersistent(player_input);
#pragma endregion

#pragma region scene objects

		//CALL THE BUILDER
		const auto enable_builder = false;
		const auto box_scene = true;
		const auto test_scene = false;

		if (enable_builder) {
			//NEW BUILDER
			auto newbuilder = new ext::AnimoBuilder::AnimoBuilderObject();
			newbuilder->SetBaseParams({
				.pillarInterval = 6,
				.beamInterval = 3
				});
			newbuilder->SetParent(this->current_root);

			newbuilder->AddPillar(Vector3(30, 0, 2));
			newbuilder->AddPillar(Vector3(30, 0, -8));
			newbuilder->AddPillar(Vector3(24, 0, -8));
			newbuilder->AddPillar(Vector3(24, 0, -2));
			newbuilder->AddPillar(Vector3(-24, 0, -2));
			newbuilder->AddPillar(Vector3(-24, 0, -8));
			newbuilder->AddPillar(Vector3(-30, 0, -8));
			newbuilder->AddPillar(Vector3(-30, 0, 2));
			newbuilder->AddPillar(Vector3(30, 0, 2)); //loop back
		}

		if (box_scene) {
			create_primitive(RenderObject::PrimitiveType::cube, Vector3(0, -5, 0), Vector3(2), Quaternion::Euler(Vector3(0)));
			create_primitive(RenderObject::PrimitiveType::cube, Vector3(0, -5, 5), Vector3(2), Quaternion::Euler(Vector3(0)));
		}

		this->Set_state(EngineState::Edit);
#pragma endregion

#pragma endregion
#pragma region MAIN LOOP

		/// MAIN GAME LOOP
		while (!mWindow->ShouldClose())
		{
			if (this->state == Engine::EngineState::Paused) {
				if (this->timeleft_stepping > 0)
					this->_time.scale = 1;
				else
					this->_time.scale = 0;
			}

			/* Poll for and process events */
			mWindow->UpdateState();
			gbe::window::WindowEventType windoweventtype;
			while (mWindow->PollEvents(windoweventtype))
			{
				if (windoweventtype == gbe::window::WindowEventType::RESIZE) {
					auto newdimensions = mWindow->Get_dimentions();
					mRenderPipeline->SetResolution(newdimensions);
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
				}, mWindow);

			//Update GUI system

			//Early update
			this->current_root->GetHandler<EarlyUpdate>()->DoOnEnabled([](EarlyUpdate* updatable) {
				updatable->InvokeEarlyUpdate();
				});

			//Update Render pipeline
			//EDITOR UPDATE
			mEditor->PrepareFrame();
			mEditor->Update();
			//<----------MORE EDITOR FUNCTIONS GO HERE
			mEditor->PresentFrame();
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
				frustrum = current_camera->getproj() * current_camera->GetViewMat();
				viewm = current_camera->GetViewMat();
				projm = current_camera->getproj();
				nearclip = current_camera->nearClip;
				farclip = current_camera->farClip;
			}
			else {
				throw std::runtime_error("No cameras rendering scene.");
			}
			mRenderPipeline->RenderFrame(viewm, projm, nearclip, farclip);
			//mGUIPipeline->DrawActiveCanvas();

			//Update the window
			mWindow->SwapBuffers();

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

				//GUI
				//mGUIPipeline->Update(deltatime);
				};
			this->_time.TickFixed(onTick);

			mInputSystem->ResetStates(mWindow);

			//Queued root change
			if (this->queued_rootchange != nullptr) {
				mEditor->PrepareSceneChange();

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
		mRenderPipeline->CleanUp();
		mWindow->Terminate();
	}
}