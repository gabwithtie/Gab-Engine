#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace gbe {
	struct SerializedObject {
		std::string type;
		bool enabled;
		float local_position[3];
		float local_scale[3];
		float local_euler_rotation[3];
		std::unordered_map<std::string, std::string> serialized_variables;
		std::vector<SerializedObject> children;
	};
}