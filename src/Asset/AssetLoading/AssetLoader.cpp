#include "AssetLoader.h"

#include "Editor/gbe_editor.h"

std::vector<gbe::asset::AssetLoader_base_base*> gbe::asset::all_asset_loaders;

gbe::editor::InspectorData* gbe::asset::GetInspectorData(std::filesystem::path path) {
	for (const auto& assetloader : all_asset_loaders)
	{
		auto assetdata = assetloader->GetBaseAssetData(path);

		if (assetdata == nullptr)
			continue;

		return assetdata->GetInspectorData();
	}

	return nullptr;
}