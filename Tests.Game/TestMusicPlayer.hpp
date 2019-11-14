#pragma once

class TestMusicPlayer
{
public:
	Audio* audio;
	Ref<AudioStream> song;

public:
	TestMusicPlayer();
	virtual void Init(const String& songPath, uint32 startOffset);
	void Run();
	virtual void Update(float dt);;
};