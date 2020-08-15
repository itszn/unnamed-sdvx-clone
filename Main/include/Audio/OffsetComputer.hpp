#pragma once
#include <Audio/AudioStream.hpp>
#include <Beatmap/BeatmapObjects.hpp>

class Beatmap;

class OffsetComputer
{
public:
	OffsetComputer(Ref<AudioStream> music, const Beatmap& beatmap);
	OffsetComputer(const OffsetComputer&) = delete;
	OffsetComputer(OffsetComputer&&) = delete;

	OffsetComputer& operator= (const OffsetComputer&) = delete;
	OffsetComputer& operator= (OffsetComputer&&) = delete;

	bool Compute(int& outOffset);

private:
	// Length of the region to use for offset computation
	static constexpr MapTime COMPUTE_WINDOW = 20'000;

	// Maximal absolute value for offset
	static constexpr MapTime MAX_OFFSET = 50;

	const float* m_pcm = nullptr;
	uint64 m_pcmCount = 0;
	uint32 m_sampleRate = 0;
	const Beatmap& m_beatmap;

	void ReadBeats();
	Vector<MapTime> m_beats;

	void ComputeEnergy();
	MapTime m_energyOffset;
	Vector<float> m_energy;
	Vector<float> m_onsetScore;

	int ComputeFitness(MapTime offset);
	int ComputeOnsetScore(MapTime time);
};