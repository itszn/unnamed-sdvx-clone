#include "stdafx.h"
#include "AudioStream.hpp"
#include "Audio.hpp"
#include "AudioStreamMa.hpp"
#include "AudioStreamMp3.hpp"
#include "AudioStreamOgg.hpp"
#include "AudioStreamWav.hpp"

Ref<AudioStream> AudioStream::Create(class Audio* audio, const String& path, bool preload)
{
	Ref<AudioStream> impl;

	auto TryCreateType = [&](int32 type)
	{
		if (type == 0)
			return AudioStreamOgg::Create(audio, path, preload);
		else if (type == 1)
			return AudioStreamMp3::Create(audio, path, preload);
		else
			return AudioStreamMa::Create(audio, path, preload);
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