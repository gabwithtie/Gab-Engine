#pragma once

#include <typeinfo>
#include <string>

namespace gbe {
	class Object;

	class ObjectNamer {
	public:
		static void ResetName(Object* obj);
		static std::string GetName(const type_info& info);
	};
}