#pragma once


#include "Objects/Root.h"

//EXTERNALS
#include "Ext/AnimoBuilderWrapper/AnimoBuilderWrapper.h"
#include "Ext/AnimoBuilder/AnimoBuilder.h"

namespace gbe {
	class Engine {
	private:
		static Engine* instance;

		//Current Root and its handlers
		Root* current_root;
		Root* queued_rootchange;

		std::vector<Object*> persistents;
	public:
		Engine();
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