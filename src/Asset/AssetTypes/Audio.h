#pragma once

#include "Asset/BaseAsset.h"
#include <functional>

#include "Math/gbe_math.h"

namespace gbe {
	namespace asset {
		namespace data {
			struct AudioImportData {
				std::string filename;
			};
		}

		class Audio : public BaseAsset<Audio, data::AudioImportData> {
		public:
			Audio(std::string path);
			void Play(Vector3 position = Vector3::zero, float volume = 1.0f, int channel = 0);
		};
	}
}