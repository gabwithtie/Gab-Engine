#pragma once

#include <typeinfo>
#include <unordered_map>
#include <string>
#include <functional>

#include "Engine/gbe_engine.h"

namespace gbe::editor {
	class CreateFunctions {
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions;
	public:

	};
}