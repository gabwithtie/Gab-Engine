#pragma once

#include "Engine/gbe_engine.h"
#include "Asset/gbe_asset.h"
#include <string>
#include <unordered_map>
#include <functional>

#include <typeinfo>

namespace gbe::editor {
	class TypeSerializer {
	private:
		static const std::unordered_map<std::string, std::function<Object*>> instantiation_dictionary;
	public:
		static void InitializeTypeDictionary();
		Object* Instantiate(std::string type_id);
	};
}