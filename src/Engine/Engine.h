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
	public:
		Engine();
		static bool ChangeRoot(Root* newroot);
		static Root* CreateBlankRoot();
		static Root* GetCurrentRoot();
		void Run();
	};
}