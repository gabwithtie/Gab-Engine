#pragma once

#include <functional>
#include <string>

namespace gbe {
	namespace asset {
		namespace internal {
			class BaseAsset_base;
		}

		template<class TAsset, class TAssetImportData, class TAssetLoadData>
		class AssetLoader_base {
		protected:
			static AssetLoader_base* active_base_instance;
			std::function<bool(TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data)> load_func;
			std::function<void(TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data)> unload_func;
		public:
			static bool LoadAsset(TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data) {
				if (active_base_instance == nullptr)
					throw "asset loader for this particular type is not assigned!";

				return active_base_instance->load_func(asset, import_data, load_data);
			}
			static void UnLoadAsset(TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data) {
				if (active_base_instance == nullptr)
					throw "asset loader for this particular type is not assigned!";

				return active_base_instance->unload_func(asset, import_data, load_data);
			}
		};

		template<class TAsset, class TAssetImportData, class TAssetLoadData>
		AssetLoader_base<TAsset, TAssetImportData, TAssetLoadData>* AssetLoader_base<TAsset, TAssetImportData, TAssetLoadData>::active_base_instance = nullptr;

		template<class TAsset, class TAssetImportData, class TAssetLoadData, class TAssetRuntimeData>
		class AssetLoader : AssetLoader_base<TAsset, TAssetImportData, TAssetLoadData> {
		protected:
			static AssetLoader* active_instance;
			std::unordered_map<TAsset*, TAssetRuntimeData> loaded_assets;
			virtual TAssetRuntimeData LoadAsset_(TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data) = 0;
			virtual void UnLoadAsset_(TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data) = 0;
		public:
			virtual void AssignSelfAsLoader() {
				this->active_instance = this;
				this->active_base_instance = this;

				this->load_func = [](TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data) {
					auto newdata = active_instance->LoadAsset_(asset, import_data, load_data);
					active_instance->loaded_assets.insert_or_assign(asset, newdata);

					return true;
					};

				this->unload_func = [](TAsset* asset, const TAssetImportData& import_data, TAssetLoadData* load_data) {
					auto it = active_instance->loaded_assets.find(asset);
					if (it != active_instance->loaded_assets.end()) {
						active_instance->UnLoadAsset_(asset, import_data, load_data);
						active_instance->loaded_assets.erase(it);
					}
					};
			}

			static TAssetRuntimeData& GetAssetData(TAsset* assetinstance) {
				auto it = active_instance->loaded_assets.find(assetinstance);
				if (it != active_instance->loaded_assets.end()) {
					return it->second;
				}
				else {
					throw std::exception("Asset not found");
				}
			}

			static TAsset* GetAsset(std::string asset_id) {
				for (const auto& pair : active_instance->loaded_assets) {
					if (pair.first->Get_assetId() == asset_id) {
						return pair.first;
					}
				}
				throw std::exception("Asset not found");
			}

			static TAsset* GetAssetByPath(std::string asset_path) {
				for (const auto& pair : active_instance->loaded_assets) {
					if (pair.first->Get_asset_filepath() == asset_path) {
						return pair.first;
					}
				}
				throw std::exception("Asset not found");
			}

			static std::vector<internal::BaseAsset_base*> GetAssetList() {
				std::vector<internal::BaseAsset_base*> list;

				for (const auto& pair : active_instance->loaded_assets)
				{
					list.push_back(pair.first);
				}

				return list;
			}
		};

		template<class TAsset, class TAssetImportData, class TAssetLoadData, class TAssetRuntimeData>
		AssetLoader<TAsset, TAssetImportData, TAssetLoadData, TAssetRuntimeData>* AssetLoader<TAsset, TAssetImportData, TAssetLoadData, TAssetRuntimeData>::active_instance = nullptr;
	}
}