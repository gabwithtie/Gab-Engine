#pragma once


#include "Objects/Root.h"
#include "Global/Time.h"

//EXTERNALS
#include "Ext/AnimoBuilderWrapper/AnimoBuilderWrapper.h"
#include "Ext/AnimoBuilder/AnimoBuilder.h"

namespace gbe {
	class Engine {
	public:
		enum EngineState {
			Edit,
			Play,
			Paused
		};
	private:
		static Engine* instance;

		//Current Root and its handlers
		Root* current_root;
		Root* queued_rootchange;

		std::vector<Object*> persistents;

		EngineState state;

		SerializedObject* pre_play_scenedata = nullptr;

		Time _time;
	public:
		Engine();

		inline EngineState Get_state() {
			return this->state;
		}
		static void Set_state(EngineState _state);

		static bool ChangeRoot(Root* newroot);
		static Root* CreateBlankRoot();
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