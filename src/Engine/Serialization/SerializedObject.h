#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace gbe {
	namespace editor {
		class SerializedObject {
			std::string type;
			float local_position[3];
			float local_scale[3];
			float local_rotaion[3];
			std::unordered_map<std::string, std::string> serialized_variables;
			std::vector<SerializedObject> children;
		};
	}
}