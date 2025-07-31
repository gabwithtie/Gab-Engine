#include "TypeSerializer.h"

namespace gbe::editor {
	std::unordered_map<std::string, std::function<Object*()>> instantiation_dictionary;
	
	void TypeSerializer::InitializeTypeDictionary()
	{
	}

	Object* TypeSerializer::Instantiate(std::string type_id)
	{
		return nullptr;
	}
}
