#include "AudioLoader.h"

void gbe::audio::AudioLoader::LoadAsset_(asset::Audio* asset, const asset::data::AudioImportData& importdata, AudioData* data)
{
	auto directory = asset->Get_asset_filepath() / importdata.filename;
	auto new_audio_data = Mix_LoadWAV(directory.string().c_str());
	if (new_audio_data == nullptr)
		throw std::exception("Audio Failed to Load!");
}

void gbe::audio::AudioLoader::UnLoadAsset_(AudioData* data)
{

}
