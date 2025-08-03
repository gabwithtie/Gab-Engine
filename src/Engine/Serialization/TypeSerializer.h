#pragma once

#include "SerializedObject.h"

#include "Engine/gbe_engine.h"
#include "Asset/gbe_asset.h"
#include <string>
#include <unordered_map>
#include <functional>

#include <typeinfo>

namespace gbe {
	class TypeSerializer {
	private:
		static std::unordered_map<std::string, std::function<Object*(SerializedObject)>> instantiation_dictionary;
	public:
		static void RegisterTypeCreator(std::string type_id, std::function<Object* (gbe::SerializedObject)> instantiation_function);
		static Object* Instantiate(std::string type_id, SerializedObject data);
	};
}