#pragma once

#include "Global/Time.h"
#include "Objects/Root.h"
#include "Window/gbe_window.h"
#include "Graphics/gbe_graphics.h"

//EXTERNALS
#include "Ext/AnimoBuilderWrapper/AnimoBuilderWrapper.h"
#include "Ext/AnimoBuilder/AnimoBuilder.h"

namespace gbe {
	class Camera;
	class Editor;

	class Engine {
	public:
		enum EngineState {
			Edit,
			Play,
			Paused
		};
	private:
		static Engine* instance;

		//COMPONENT OBJECTS
		Window window;
		RenderPipeline renderpipeline;

		//EDITOR
		Editor* editor = nullptr;

		//TIME
		Time time;
		double timeleft_stepping = 0;
		EngineState state = EngineState::Edit;

		//SCENE MANAGEMENT
		SerializedObject* pre_play_scenedata = nullptr;
		Root* current_root = nullptr;
		Root* queued_rootchange = nullptr;
		std::vector<Object*> persistents;
		void InitializeRoot();
	public:
		Engine();

		inline EngineState Get_state() {
			return this->state;
		}
		static void Set_state(EngineState _state);
		static void Step(double dur);
		static bool ChangeRoot(Root* newroot);
		static Root* CreateBlankRoot();
		static Camera* GetActiveCamera();
		inline static Root* GetCurrentRoot() {
			return instance->current_root;
		}
		inline static void MakePersistent(Object* something) {
			if (something->GetParent() != instance->current_root)
				throw std::runtime_error("Cannot make persistent a non-root object.");

			instance->persistents.push_back(something);
		}
		void Run();
	};
}