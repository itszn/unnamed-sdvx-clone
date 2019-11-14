#include "stdafx.h"
#include "AudioStreamBase.hpp"
extern "C"
{
	#include "minimp3.h"
}

class AudioStreamMp3 : public AudioStreamBase
{
	mp3_decoder_t* m_decoder;
	size_t m_mp3dataOffset = 0;
	size_t m_mp3dataLength = 0;
	int32 m_mp3samplePosition = 0;
	int32 m_samplingRate = 0;
	uint8* m_dataSource = 0;

	Map<int32, size_t> m_frameIndices;
	uint32 m_largetsFrameIndex;
	Vector<float> m_pcm;
	int64 m_playPos;

	bool m_firstFrame = true;
	int m_unsynchsafe(int in);
	int m_toLittleEndian(int num);
protected:
	AudioStreamMp3() = default;
	~AudioStreamMp3();
	bool Init(Audio* audio, const String& path, bool preload) override;
	void SetPosition_Internal(int32 pos) override;
	int32 GetStreamPosition_Internal() override;
	int32 GetStreamRate_Internal() override;
	uint32 GetSampleRate_Internal() const override;
	float* GetPCM_Internal() override;
	int32 DecodeData_Internal() override;
public:
	static Ref<AudioStream> Create(Audio* audio, const String& path, bool preload);
};
