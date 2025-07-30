#include "TypeSerializer.h"

namespace gbe::editor::TypeSerializer {
	std::unordered_map<std::string, std::function<Object*>> instantiation_dictionary;
	
	void InitializeTypeDictionary()
	{
	}

	Object* Instantiate(std::string type_id)
	{
		return nullptr;
	}
}
