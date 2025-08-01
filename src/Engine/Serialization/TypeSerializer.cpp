#include "TypeSerializer.h"

namespace gbe {
	std::unordered_map<std::string, std::function<Object*()>> TypeSerializer::instantiation_dictionary;
	
	void TypeSerializer::InitializeTypeDictionary()
	{

	}

	void TypeSerializer::RegisterTypeCreator(std::string type_id, std::function<Object* ()> instantiation_function)
	{
		if (instantiation_dictionary.find(type_id) == instantiation_dictionary.end()) {
			instantiation_dictionary.insert_or_assign(type_id, instantiation_function);
		}
	}

	Object* TypeSerializer::Instantiate(std::string type_id)
	{
		if (instantiation_dictionary.find(type_id) == instantiation_dictionary.end())
			return nullptr;

		return instantiation_dictionary[type_id]();
	}
}
