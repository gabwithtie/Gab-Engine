#pragma once

#include <filesystem>
#include <string>

#include "Asset/gbe_asset.h"

namespace gbe::editor {
	struct ProjectInfo {
		std::string entry;
	};

	class ProjectLoader {
		static std::filesystem::path currentproject_dir;

	public:
		static void Load(std::filesystem::path path);
		inline static std::filesystem::path GetCurrentProjectPath() {
			return currentproject_dir;
		}
	};
}