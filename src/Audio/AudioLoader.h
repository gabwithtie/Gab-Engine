#pragma once

#include "Asset/AssetLoading/AssetLoader.h"
#include "Asset/AssetTypes/Audio.h"
#include <unordered_map>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

namespace gbe {
	namespace audio {
		struct AudioData {
			Mix_Chunk* audio_chunk;
		};

		class AudioLoader : public asset::AssetLoader<asset::Audio, asset::data::AudioImportData, AudioData> {
		private:
		protected:
			void LoadAsset_(asset::Audio* asset, const asset::data::AudioImportData& importdata, AudioData* data) override;
			void UnLoadAsset_(AudioData* data) override;
			inline virtual void OnAsyncTaskCompleted(AsyncLoadTask* loadtask) override {
				//This is a synchronous loader
			}
		};
	}
}