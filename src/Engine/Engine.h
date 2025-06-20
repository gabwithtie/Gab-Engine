#pragma once


#include "Objects/Root.h"

//EXTERNALS
#include "Ext/AnimoBuilder/AnimoBuilder.h"

namespace gbe {
	class Engine {
	private:
		static Engine* Instance;

		//Current Root and its handlers
		Root* current_root;
		Root* queued_rootchange;
	public:
		Engine();
		bool ChangeRoot(Root* newroot);
		Root* CreateBlankRoot();
		static Root* GetCurrentRoot();
		void Run();
	};
}