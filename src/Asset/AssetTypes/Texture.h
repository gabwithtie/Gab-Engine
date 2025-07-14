#pragma once

#include "Asset/BaseAsset.h"
#include "Math/gbe_math.h"

namespace gbe {
	namespace asset {

		namespace data {
			struct TextureImportData
			{
				std::string filename;
				std::string type;
			};
			struct TextureLoadData {
				//TODO: ADD a 2D array std::vector<std::vector<Vector3>> pixels

				Vector2Int dimensions;
				int colorchannels;
			};
		}

		class Texture : public BaseAsset<Texture, data::TextureImportData, data::TextureLoadData> {
		public:
			Texture(std::string asset_path);
		};
	}
}