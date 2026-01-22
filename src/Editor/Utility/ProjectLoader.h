#pragma once

#include <filesystem>
#include <string>

#include "Asset/gbe_asset.h"

namespace gbe::editor {
	struct ProjectInfo {
		std::string entry;
	};

	class ProjectLoader {
	public:
		inline static void Load(std::filesystem::path path) {
			ProjectInfo newinfo;

			asset::serialization::gbeParser::PopulateClass(newinfo, path);

			auto tolocal = path.parent_path();
			auto fullPath = tolocal / newinfo.entry;

			asset::BatchLoader::LoadAssetsFromDirectory(tolocal);

			SerializedObject data;
			gbe::asset::serialization::gbeParser::PopulateClass(data, fullPath);
			auto newroot = gbe::Engine::CreateBlankRoot(&data);

			gbe::Engine::ChangeRoot(newroot);
		}
	};
}