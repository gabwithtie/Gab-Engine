#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "Asset/AssetLoading/AssetLoader.h"
#include "Asset/File/gbeParser.h"

namespace fs = std::filesystem;

namespace gbe {
	namespace asset {
		struct BaseImportData {
			std::string asset_type;
			std::string asset_id;
		};

		namespace internal {
			class BaseAsset_base {
			protected:
				std::filesystem::path asset_filepath;
				bool destroy_queued;
				BaseImportData base_import_data;
			public:
				inline std::filesystem::path Get_asset_filepath() {
					return asset_filepath;
				}
			};
		}

		template<class TFinal, class TImportData, class TLoadData>
		class BaseAsset : public internal::BaseAsset_base {
		protected:
			TImportData import_data;

			TLoadData load_data;
		public:
			void SetLoadData(TLoadData newload_data) {
				load_data = newload_data;
			}

			BaseAsset(std::filesystem::path asset_path) {
				gbe::asset::serialization::gbeParser::PopulateClass(this->import_data, asset_path);
				
				this->asset_filepath = asset_path;

				std::string filename_with_ext = asset_path.filename().string();
				size_t dot_pos = filename_with_ext.find('.');
				if (dot_pos != std::string::npos)
					this->base_import_data.asset_id = filename_with_ext.substr(0, dot_pos);
				else
					this->base_import_data.asset_id = filename_with_ext;
				
				AssetLoader_base<TFinal, TImportData, TLoadData>::LoadAsset(static_cast<TFinal*>(this), this->import_data, &this->load_data);
			}
			std::string Get_assetId() {
				return this->base_import_data.asset_id;
			}
			bool Get_destroy_queued() {
				return this->destroy_queued;
			}
			const TLoadData& Get_load_data() {
				return this->load_data;
			}
			inline static TFinal* GetAssetById(std::string id) {
				return AssetLoader_base<TFinal, TImportData, TLoadData>::GetAsset(id);
			}
		};
	}
}