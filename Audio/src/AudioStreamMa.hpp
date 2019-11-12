#include "stdafx.h"
#include "AudioStreamBase.hpp"

#define DR_WAV_IMPLEMENTATION
#include "extras/dr_wav.h"   // Enables WAV decoding.
#define DR_FLAC_IMPLEMENTATION
#include "extras/dr_flac.h"  // Enables FLAC decoding.
#define DR_MP3_IMPLEMENTATION
#include "extras/dr_mp3.h"   // Enables MP3 decoding.

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

class AudioStreamMa : public AudioStreamBase
{
private:
	Buffer m_Internaldata;
	float* m_pcm = nullptr;

	int64 m_playbackPointer = 0;
	uint64 m_dataPosition = 0;

	const int sample_rate = 48000;
	ma_decoder m_decoder;
	int m_byteRate;

public:
	~AudioStreamMa();
	bool Init(Audio* audio, const String& path, bool preload) override;
	int32 GetStreamPosition_Internal() override;
	int32 GetStreamRate_Internal() override;
	void SetPosition_Internal(int32 pos) override;
	int32 DecodeData_Internal() override;
	float* GetPCM_Internal() override;
	uint32 GetSampleRate_Internal() const override;
};

class AudioStreamRes* CreateAudioStream_ma(class Audio* audio, const String& path, bool preload);
