#include "AssetLoader.h"

#include "Editor/gbe_editor.h"

std::unordered_map<gbe::asset::AssetType, gbe::asset::AssetLoader_base_base*> gbe::asset::all_asset_loaders;

gbe::asset::internal::BaseAsset_base* gbe::asset::GetBaseData(std::filesystem::path path) {
	for (const auto& lpair : all_asset_loaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata;
	}

	return nullptr;
}

gbe::editor::InspectorData* gbe::asset::GetInspectorData(std::filesystem::path path) {
	for (const auto& lpair : all_asset_loaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata->GetInspectorData();
	}

	return nullptr;
}

gbe::asset::AssetType gbe::asset::GetAssetType(std::filesystem::path path) {
	for (const auto& lpair : all_asset_loaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata->Get_assettype();
	}

	return AssetType::NONE;
}

std::string gbe::asset::GetAssetId(std::filesystem::path path) {
	for (const auto& lpair : all_asset_loaders)
	{
		const auto& assetloader = lpair.second;
		auto assetdata = assetloader->FindAssetByPath(path);

		if (assetdata == nullptr)
			continue;

		return assetdata->Get_assetId();
	}

	return "";
}