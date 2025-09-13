#include "Texture.h"
#include "Asset/AssetLoading/AssetLoader.h"

gbe::asset::Texture::Texture(std::filesystem::path asset_path) : gbe::asset::BaseAsset<Texture, data::TextureImportData, data::TextureLoadData>(asset_path){

}