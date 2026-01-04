#pragma once

#include <typeinfo>
#include <unordered_map>
#include <string>
#include <functional>

#include "Engine/gbe_engine.h"

namespace gbe::editor {
	class CreateFunctions {
	public:
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions_primitives;
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions_light;
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions_ext_anitobuilder;
	};
}