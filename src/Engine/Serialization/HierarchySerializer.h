#pragma once

#include "Engine/gbe_engine.h"
#include "Asset/gbe_asset.h"
#include <string>

namespace gbe {
	class HierarchySerializer {
	private:
		Object* hierarchy_root;
	public:
		HierarchySerializer(Object* hierarchy_root);
		std::string Serialize(std::string path);
		Object* DeserializeAndParent(std::string path);
	};
}