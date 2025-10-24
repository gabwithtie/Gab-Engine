#pragma once

#include <typeinfo>
#include <unordered_map>
#include <string>
#include <functional>

#include "Engine/gbe_engine.h"

namespace gbe::editor {
	class CreateFunctions {
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions_primitives;
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions_light;
		const static std::unordered_map<std::string, std::function<Object* ()>> createfunctions_ext_anitobuilder;
	public:
		inline static const std::unordered_map<std::string, std::function<Object* ()>>& GetCreators_primitives() {
			return createfunctions_primitives;
		}
		inline static const std::unordered_map<std::string, std::function<Object* ()>>& GetCreators_light() {
			return createfunctions_light;
		}
		inline static const std::unordered_map<std::string, std::function<Object* ()>>& GetCreators_anitobuilder() {
			return createfunctions_ext_anitobuilder;
		}
	};
}