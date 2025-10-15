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
		typedef std::function<Object* (SerializedObject*)> TypeCreatorFunction;

	private:
		static std::unordered_map<std::string, TypeCreatorFunction> instantiation_dictionary;
	public:
		static void RegisterTypeCreator(std::string type_id, TypeCreatorFunction instantiation_function);
		static Object* Instantiate(std::string type_id, SerializedObject* data);
	};
}