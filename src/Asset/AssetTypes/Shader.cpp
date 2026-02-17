#include "Shader.h"

#include "Asset/AssetLoading/AssetLoader.h"
#include "Asset/AssetTypes/Texture.h"

gbe::asset::Shader::Shader(std::filesystem::path path) : BaseAsset(path)
{
}