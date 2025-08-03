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
		root_object->RegisterHandler(new PhysicsHandler(physics::PhysicsPipeline::Get_Instance()));
		root_object->RegisterHandler(new ColliderHandler(physics::PhysicsPipeline::Get_Instance()));
		root_object->RegisterHandler(new ObjectHandler<gbe::LightObject>());
		root_object->RegisterHandler(new ObjectHandler<gbe::Camera>());
		root_object->RegisterHandler(new ObjectHandler<PhysicsUpdate>());
		root_object->RegisterHandler(new ObjectHandler<InputPlayer>());
		root_object->RegisterHandler(new ObjectHandler<EarlyUpdate>());
		root_object->RegisterHandler(new ObjectHandler<Update>());
		root_object->RegisterHandler(new ObjectHandler<LateUpdate>());

		return root_object;
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
		auto mTime = new Time();
#pragma region Physics Pipeline Setup
		auto mPhysicsPipeline = new physics::PhysicsPipeline();
		mPhysicsPipeline->Init();
		mPhysicsPipeline->Set_OnFixedUpdate_callback(
			[=](float physicsdeltatime) {
				auto phandler = this->current_root->GetHandler<PhysicsUpdate>();
				for (auto updatable : phandler->object_list)
				{
					updatable->InvokePhysicsUpdate(physicsdeltatime);
				}
			}
		);
#pragma endregion
#pragma region Audio Pipeline Setup
		auto mAudioPipeline = new audio::AudioPipeline();
		//mAudioPipeline->Init();
#pragma endregion
#pragma region Editor Setup
		auto mEditor = new gbe::Editor(mRenderPipeline, mWindow, this, mTime);
		mWindow->AddAdditionalEventProcessor([mEditor](void* newevent) {
			mEditor->ProcessRawWindowEvent(newevent);
			});
#pragma endregion
#pragma region Asset Loading
		//AUDIO CACHING

		//MESH CACHING
		auto cube_mesh = new asset::Mesh("DefaultAssets/3D/default/cube.obj.gbe");
		auto sphere_mesh = new asset::Mesh("DefaultAssets/3D/default/sphere.obj.gbe");
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

		//TYPE SERIALIZER REGISTERING
		gbe::TypeSerializer::RegisterTypeCreator(typeid(RenderObject).name(), RenderObject::Create);
		gbe::TypeSerializer::RegisterTypeCreator(typeid(RigidObject).name(), RigidObject::Create);
		gbe::TypeSerializer::RegisterTypeCreator(typeid(BoxCollider).name(), BoxCollider::Create);
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

#pragma region scene singletons
		//forward declaration
		auto player = new RigidObject();

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

		//Global objects
		auto player_input = new InputPlayer(player_name);
		player_input->SetParent(this->current_root);
		auto camera_controller = new FlyingCameraControl();
		camera_controller->SetParent(player_input);

		PerspectiveCamera* player_cam = new PerspectiveCamera(mWindow);
		player_cam->SetParent(camera_controller);

		Engine::MakePersistent(player_input);

		//================INPUT HANDLING================//
		auto input_communicator = new GenericController();
		input_communicator->SetParent(player_input);
		//Left click customer
		input_communicator->AddCustomer(new InputCustomer<KeyPress<Keys::MOUSE_LEFT>>([&](KeyPress<Keys::MOUSE_LEFT>* value, bool changed) {
			}));
		//WASD customer
		input_communicator->AddCustomer(new InputCustomer<WasdDelta>([=](WasdDelta* value, bool changed) {

			}));
		//Spacebar customer
		input_communicator->AddCustomer(new InputCustomer<KeyPress<Keys::SPACE>>([=](KeyPress<Keys::SPACE>* value, bool changed) {
			if (value->state != KeyPress<Keys::SPACE>::START)
				return;

			const bool spawn_merged = false;

			if (spawn_merged) {
				auto merged_mesh = new asset::Mesh("out/merged.obj.gbe");
				auto merged_dc = mRenderPipeline->RegisterDrawCall(merged_mesh, cube_mat);

				auto current_camera = this->GetCurrentRoot()->GetHandler<Camera>()->object_list.front();
				Vector3 camera_pos = current_camera->World().position.Get();
				auto mousedir = current_camera->ScreenToRay(Vector2(0, 0));

				create_mesh(merged_dc, camera_pos + (mousedir * 20.0f), Vector3(1, 1, 1), Quaternion::Euler(Vector3::zero));
			}

			const bool spawn_boxes = true;

			if (spawn_boxes) {
				for (size_t i = 0; i < 15; i++)
				{
					Vector3 from = Vector3(2, 30, 2);
					Vector3 to = -from;
					to.y = from.y;
					create_primitive(RenderObject::PrimitiveType::cube, Vector3::RandomWithin(from, to), Vector3(1.0f));
				}
			}

			}));
		//ESCAPE Customer
		input_communicator->AddCustomer(new InputCustomer<KeyPress<Keys::ESCAPE>>([=](KeyPress<Keys::ESCAPE>* value, bool changed) {
			if (value->state != KeyPress<Keys::ESCAPE>::START)
				return;

			}));
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

		if (test_scene) {
			//MESH AND DRAWCALLS FOR TEST
			auto brick_mat = new asset::Material("DefaultAssets/Materials/unlit.mat.gbe");
			auto brick_tex = new asset::Texture("DefaultAssets/Tex/Maps/Model/brick.img.gbe");
			brick_mat->setOverride("colortex", brick_tex);

			auto teapot_m = new asset::Mesh("DefaultAssets/3D/test/teapot.obj.gbe");
			auto teapot_dc = mRenderPipeline->RegisterDrawCall(teapot_m, brick_mat);

			auto bunny_m = new asset::Mesh("DefaultAssets/3D/test/bunny.obj.gbe");
			auto bunny_dc = mRenderPipeline->RegisterDrawCall(bunny_m, grid_mat);

			auto armadillo_m = new asset::Mesh("DefaultAssets/3D/test/armadillo.obj.gbe");
			auto armadillo_dc = mRenderPipeline->RegisterDrawCall(armadillo_m, grid_mat);

			const int trial = 3;

			if (trial == 0)
				create_mesh(teapot_dc, Vector3(0, 0, 0), Vector3(4.0f));
			if (trial == 1)
				create_mesh(bunny_dc, Vector3(-0, 0, 0), Vector3(20.0f));
			if (trial == 2)
				create_mesh(armadillo_dc, Vector3(-0, 0, 0), Vector3(2.0f));

			if (trial == 3) {
				create_mesh(teapot_dc, Vector3(0, 0, 0), Vector3(4.0f));
				create_mesh(bunny_dc, Vector3(-10, 0, 0), Vector3(20.0f));
				create_mesh(armadillo_dc, Vector3(-20, 0, 0), Vector3(2.0f));
			}
		}

#pragma endregion

#pragma endregion
#pragma region MAIN LOOP

		/// MAIN GAME LOOP
		while (!mWindow->ShouldClose())
		{
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
				for (auto input_player : inputhandler->object_list) {
					if (input_player->get_player_name() != name)
						continue;

					for (auto controller : input_player->controllers.object_list)
						controller->ForEach_inputreceivers([action, changed](InputCustomer_base* input_customer) {
						input_customer->TryReceive(action, changed);
							});
				}
				}, mWindow);

			//Update GUI system

			//Early update
			for (auto updatable : this->current_root->GetHandler<EarlyUpdate>()->object_list)
			{
				updatable->InvokeEarlyUpdate();
			}

			//Update Render pipeline
			//EDITOR UPDATE
			mEditor->PrepareFrame();
			mEditor->Update();
			//<----------MORE EDITOR FUNCTIONS GO HERE
			mEditor->PresentFrame();
			//ENGINE UPDATE
			for (auto light : this->current_root->GetHandler<LightObject>()->object_list)
			{
				if (mRenderPipeline->TryPushLight(light->GetData(), false) == false) {
					break;
				}
			}

			auto pos = Vector3::zero;
			auto forward = Vector3::zero;
			auto frustrum = Matrix4();
			auto viewm = Matrix4();
			auto projm = Matrix4();
			auto nearclip = 0.0f;
			auto farclip = 0.0f;

			if (this->current_root->GetHandler<Camera>()->object_list.size() > 0) {
				auto current_camera = this->current_root->GetHandler<Camera>()->object_list.front();
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
				physics::PhysicsPipeline::Get_Instance()->Tick(deltatime);
				physicshandler->Update();

				float delta_f = (float)deltatime;

				//Normal Update
				for (auto updatable : updatehandler->object_list)
				{
					updatable->InvokeUpdate(delta_f);
				}
				//Late Update
				for (auto updatable : lateupdatehandler->object_list)
				{
					updatable->InvokeLateUpdate(delta_f);
				}

				//GUI
				//mGUIPipeline->Update(deltatime);
				};
			mTime->TickFixed(onTick);

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
				//Object handlers setup
				this->current_root = this->queued_rootchange;
				this->queued_rootchange = nullptr;

				for (const auto persistent : this->persistents)
				{
					persistent->SetParent(this->current_root);
				}
			}
		}
#pragma endregion
		mRenderPipeline->CleanUp();
		mWindow->Terminate();
	}
}