#include "stdafx.h"
#include "Audio/OffsetComputer.hpp"

#include <Audio/Audio_Impl.hpp>
#include <Beatmap/Beatmap.hpp>
#include <Shared/Profiling.hpp>

OffsetComputer::OffsetComputer(Ref<AudioStream> music, const Beatmap& beatmap)
	: m_pcm(music->GetPCM()), m_pcmCount(music->GetPCMCount()), m_sampleRate(music->GetSampleRate()),
	m_beatmap(beatmap)
{
}

bool OffsetComputer::Compute(int& outOffset)
{
	ProfilerScope $("OffsetComputer::Compute");

	if (!m_pcm || m_pcmCount <= 0 || m_sampleRate <= 0)
	{
		Log("OffsetComputer::Compute: The stream is not loaded!", Logger::Severity::Info);
		return false;
	}

	ReadBeats();

	if (m_beats.empty())
	{
		Log("OffsetComputer::Compute: # of beats is zero!", Logger::Severity::Info);
		return false;
	}

	Logf("OffsetComputer::Compute: Using %d beats starting from %d...", Logger::Severity::Info,
		m_beats.size(), m_beats[0]);

	ComputeEnergy();

	// Doing in the most naive way for proof-of-concept implementation
	MapTime maxOffset = 0;
	int maxFitness = ComputeFitness(0);
	for(MapTime offset = -MAX_OFFSET; offset <= MAX_OFFSET; ++offset)
	{
		if (offset == 0) continue;

		const int fitness = ComputeFitness(offset);
		if (fitness > maxFitness)
		{
			maxFitness = fitness;
			maxOffset = offset;
		}
	}

	Logf("OffsetComputer::Compute: Determined offset: %d (fitness = %d)", Logger::Severity::Info, maxOffset, maxFitness);

	outOffset = static_cast<int>(maxOffset);
	return true;
}

// Read beats based on m_beatmap.
void OffsetComputer::ReadBeats()
{
	m_beats.clear();

	MapTime lastBeat = -1;

	int regionBeginInd = 0;
	int regionEndInd = 0;

	int maxBeatsBeginInd = 0;
	int maxBeatsCount = 0;

	for (const ObjectState* object : m_beatmap.GetLinearObjects())
	{
		MapTime currBeat = lastBeat;
		switch (object->type)
		{
		case ObjectType::Single:
		case ObjectType::Hold:
			currBeat = object->time;;
			break;
		default:
			continue;
			break;
		}
		if (currBeat == lastBeat) continue;

		while (regionBeginInd < regionEndInd && COMPUTE_WINDOW <= currBeat - m_beats[regionBeginInd])
		{
			++regionBeginInd;
		}

		m_beats.emplace_back(currBeat);
		++regionEndInd;

		if (regionEndInd - regionBeginInd > maxBeatsCount)
		{
			maxBeatsCount = regionEndInd - regionBeginInd;
			maxBeatsBeginInd = regionBeginInd;
		}

		lastBeat = currBeat;
	}

	if (m_beats.empty()) return;
	assert(maxBeatsCount > 0);

	for (int i = 0; i < maxBeatsCount; ++i)
	{
		const MapTime currBeat = m_beats[i] = m_beats[maxBeatsBeginInd + i];
	}

	m_beats.resize(maxBeatsCount);
}

static inline float GetSmoothValue(const float* pcm, const uint64 count, const uint64 ind)
{
	const float prev = 0 <= ind - 2 && ind - 2 < count ? pcm[ind - 2] : 0;
	const float curr = 0 <= ind && ind < count ? pcm[ind] : 0;
	const float next = 0 <= ind + 2 && ind + 2 < count ? pcm[ind + 2] : 0;

	return curr + (prev + next) * 0.5f;
}

static inline float GetAmplitude(const float* pcm, const uint64 count, const uint64 sample)
{
	return std::hypotf(GetSmoothValue(pcm, count, 2*sample), GetSmoothValue(pcm, count, 2*sample + 1));
}

void OffsetComputer::ComputeEnergy()
{
	constexpr MapTime ENERGY_COUNT = COMPUTE_WINDOW + MAX_OFFSET * 2 + 10;
	constexpr float ENERGY_EPSILON = 0.000'001;

	m_energyOffset = m_beats[0] - MAX_OFFSET - 5;

	m_energy.clear();
	m_energy.resize(ENERGY_COUNT, 0);
	
	m_onsetScore.clear();
	m_onsetScore.resize(ENERGY_COUNT, 0);

	if (m_beats.empty()) return;

	uint64 ind = (m_energyOffset * m_sampleRate) / 1000;
	const uint64 endInd = ((m_energyOffset + ENERGY_COUNT) * m_sampleRate) / 1000;
	uint64 nextInd = ((m_energyOffset + 1) * m_sampleRate) / 1000;
	uint64 intervalSize = nextInd - ind;

	uint64 energyInd = 0;
	for (; ind < endInd; ++ind)
	{
		if (ind >= nextInd)
		{
			++energyInd;
			nextInd = ((m_energyOffset + energyInd + 1) * m_sampleRate) / 1000;
			intervalSize = nextInd - ind;

			// Check for abnormal cases
			if (intervalSize <= 0) intervalSize = 0;
			if (energyInd >= COMPUTE_WINDOW) break;
		}

		// Compute energy based on Newton's laws(?)

		const float prev = GetAmplitude(m_pcm, m_pcmCount, ind-1);
		const float curr = GetAmplitude(m_pcm, m_pcmCount, ind);
		const float next = GetAmplitude(m_pcm, m_pcmCount, ind+1);

		const float v = (next - prev) / 2;
		const float a = (prev + next - 2 * curr);
		const float energySq = v * v - curr * a;
		m_energy[energyInd] += std::sqrtf(energySq < 0 ? 0 : energySq) / intervalSize;
	}

	// This is an experimentally-determined formula.
	for (ind = 1; ind < ENERGY_COUNT; ++ind)
	{
		const float min_energy = m_energy[ind - 1] * 0.995f;
		if (m_energy[ind] < min_energy) m_energy[ind] = min_energy;
		const float prevEnergy = std::logf(std::max(m_energy[ind - 1], ENERGY_EPSILON));
		const float currEnergy = std::logf(std::max(m_energy[ind], ENERGY_EPSILON));

		m_onsetScore[ind] = (currEnergy - prevEnergy) * m_energy[ind];
	}
}

int OffsetComputer::ComputeFitness(MapTime offset)
{
	int fitness = 0;
	for (MapTime beat : m_beats)
	{
		fitness += ComputeOnsetScore(beat + offset);
	}

	return fitness;
}

int OffsetComputer::ComputeOnsetScore(MapTime time)
{
	time -= m_energyOffset;

	return 0 <= time && time < static_cast<MapTime>(m_onsetScore.size()) ? Math::Clamp(static_cast<int>(100 * m_onsetScore[time]), -100, 100) : 0;
}
