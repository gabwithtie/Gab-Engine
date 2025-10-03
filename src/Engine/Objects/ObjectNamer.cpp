#include "ObjectNamer.h"

#include <typeinfo>
#include <string>

#include "Object.h"

void gbe::ObjectNamer::ResetName(Object* obj)
{
	obj->SetName(GetName(typeid(*obj)));
}

std::string gbe::ObjectNamer::GetName(const type_info& info)
{
	std::string typeName = info.name();

	size_t lastColon = typeName.find_last_of("::");
	// Find the position of the last '::'
	if (lastColon != std::string::npos) {
		// If a '::' is found, return the substring after it
		// We add 1 to the position to skip the last ':'
		if (typeName[lastColon - 1] == ':') {
			typeName = typeName.substr(lastColon + 1);
		}
	}

	return typeName;
}
