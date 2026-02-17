#pragma once

#include "Asset/BaseAsset.h"
#include "Math/gbe_math.h"

namespace gbe {
	namespace asset {

		namespace data {
			struct TextureImportData
			{
				std::string path;
				std::string type;
			};
		}

		class Texture : public BaseAsset<Texture, data::TextureImportData> {
		public:
			Texture(std::filesystem::path asset_path);
		};
	}
}