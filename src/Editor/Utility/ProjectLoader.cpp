#include "ProjectLoader.h"

#include "Engine/gbe_engine.h"

std::filesystem::path gbe::editor::ProjectLoader::currentproject_dir;

void gbe::editor::ProjectLoader::Load(std::filesystem::path path)
{
	ProjectInfo newinfo;

	asset::serialization::gbeParser::PopulateClass(newinfo, path);

	auto tolocal = path.parent_path();
	currentproject_dir = tolocal;

	auto fullPath = tolocal / newinfo.entry;

	asset::BatchLoader::GenerateMetafiles(tolocal);
	asset::BatchLoader::LoadAssetsFromDirectory(tolocal);

	gbe::SerializedObject data;
	gbe::asset::serialization::gbeParser::PopulateClass(data, fullPath);
	auto newroot = gbe::Engine::CreateBlankRoot(&data);

	gbe::Engine::ChangeRoot(newroot);
}