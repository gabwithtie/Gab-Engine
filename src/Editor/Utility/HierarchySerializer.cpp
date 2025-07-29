#include "HierarchySerializer.h"

gbe::editor::HierarchySerializer::HierarchySerializer(Object* _hierarchy_root)
{
	this->hierarchy_root = _hierarchy_root;
}

std::string gbe::editor::HierarchySerializer::Serialize(std::string path)
{


	return std::string();
}

gbe::Object* gbe::editor::HierarchySerializer::DeserializeAndParent(std::string path)
{
	return nullptr;
}
