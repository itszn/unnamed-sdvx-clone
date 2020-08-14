#pragma once
#include <Audio/AudioStream.hpp>

class Beatmap;

class OffsetComputer
{
public:
	OffsetComputer(Ref<AudioStream> music, const Beatmap& beatmap) : m_music(music), m_beatmap(beatmap) {}
	OffsetComputer(const OffsetComputer&) = delete;
	OffsetComputer(OffsetComputer&&) = delete;

	OffsetComputer& operator= (const OffsetComputer&) = delete;
	OffsetComputer& operator= (OffsetComputer&&) = delete;

	bool Compute(int& offset);

private:
	Ref<AudioStream> m_music;
	const Beatmap& m_beatmap;
};