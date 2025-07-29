#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace gbe {
	namespace editor {
		class SerializedObject {
			std::string type;
			std::unordered_map<std::string, std::string> serialized_variables;
			std::vector<SerializedObject> children;
		};
	}
}