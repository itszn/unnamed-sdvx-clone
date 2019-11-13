#include "stdafx.h"
#include "AudioStream.hpp"
#include "Audio.hpp"
#include "Audio_Impl.hpp"

Ref<AudioStream> CreateAudioStream_ogg(class Audio* audio, const String& path, bool preload);
Ref<AudioStream> CreateAudioStream_mp3(class Audio* audio, const String& path, bool preload);
Ref<AudioStream> CreateAudioStream_wav(class Audio* audio, const String& path, bool preload);
Ref<AudioStream> CreateAudioStream_ma(class Audio* audio, const String& path, bool preload);

Ref<AudioStream> AudioStream::Create(class Audio* audio, const String& path, bool preload)
{
	Ref<AudioStream> impl;

	auto TryCreateType = [&](int32 type)
	{
		if (type == 0)
			return CreateAudioStream_ogg(audio, path, preload);
		else if (type == 1)
			return CreateAudioStream_mp3(audio, path, preload);
		else
			return CreateAudioStream_ma(audio, path, preload);
	};

	int32 pref = 0;
	String ext = Path::GetExtension(path);
	if (ext == "mp3")
		pref = 1;
	else if (ext == "ogg")
		pref = 0;
	else if (ext == "wav")
		pref = 3;

	for(uint32 i = 0; i < 3; i++)
	{
		impl = TryCreateType(pref);
		if(impl)
			break;
		pref = (pref + 1) % 3;
	}

	if(!impl)
		return Ref<AudioStream>();

	audio->GetImpl()->Register(impl.GetData());
	return Ref<AudioStream>(impl);
}