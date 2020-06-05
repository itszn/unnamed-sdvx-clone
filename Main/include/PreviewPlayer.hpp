#pragma once

#include "stdafx.h"

class AudioStream;

class PreviewPlayer
{
public:
	void FadeTo(Ref<AudioStream> stream);
	void Update(float deltaTime);
	void Pause();
	void Restore();
private:
	static const float m_fadeDuration;
	static const float m_fadeDelayDuration;
	float m_fadeInTimer = 0.0f;
	float m_fadeOutTimer = 0.0f;
	float m_fadeDelayTimer = 0.0f;
	Ref<AudioStream> m_nextStream;
	Ref<AudioStream> m_currentStream;
};

typedef struct PreviewParams
{
	String filepath;
	uint32 offset;
	uint32 duration;

	bool operator!=(const PreviewParams& rhs)
	{
		return filepath != rhs.filepath || offset != rhs.offset || duration != rhs.duration;
	}
} PreviewParams;
