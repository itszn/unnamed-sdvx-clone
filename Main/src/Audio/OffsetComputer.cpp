#include "stdafx.h"
#include "Audio/OffsetComputer.hpp"

#include <Audio/Audio_Impl.hpp>
#include <Beatmap/Beatmap.hpp>
#include <Shared/Profiling.hpp>
#include <array>

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
	if (m_energy.empty())
	{
		Log("OffsetComputer::Compute: Insufficient data...", Logger::Severity::Info);
		return false;
	}

	std::array<int, MAX_OFFSET*2 + 1> fitnesses;

	// This O(MAX_OFFSET*MAX_BEATS) is sub-optimal for computing convolution,
	// but it's simple and doesn't matter a lot in practice.
	for(MapTime offset = -MAX_OFFSET; offset <= MAX_OFFSET; ++offset)
	{
		fitnesses[MAX_OFFSET + offset] = ComputeFitness(offset);
	}

	std::vector<MapTime> peaks;

	for (MapTime offset = -MAX_OFFSET; offset <= MAX_OFFSET; ++offset)
	{
		// Only consider the peaks of fitnesses
		if (-MAX_OFFSET < offset && offset < MAX_OFFSET)
		{
			const int index = static_cast<int>(offset + MAX_OFFSET);
			if (!(fitnesses[index - 1] < fitnesses[index] && fitnesses[index] >= fitnesses[index + 1]))
				continue;
		}

		peaks.emplace_back(offset);
	}

	if (peaks.empty())
	{
		Log("OffsetComputer::Compute: Insufficient candidates...", Logger::Severity::Info);
		return false;
	}

	std::sort(peaks.begin(), peaks.end(), [&fitnesses](MapTime a, MapTime b) {
		return fitnesses[a + MAX_OFFSET] > fitnesses[b + MAX_OFFSET];
	});

	for (size_t i = 0; i < 5 && i < peaks.size(); ++i)
	{
		Logf("offset %3d | score = %d", Logger::Severity::Info, peaks[i], fitnesses[peaks[i] + MAX_OFFSET]);
	}

	Logf("OffsetComputer::Compute: Determined offset: %d (fitness = %d)", Logger::Severity::Info, peaks[0], fitnesses[peaks[0] + MAX_OFFSET]);

	outOffset = static_cast<int>(peaks[0]);
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

	if (maxBeatsCount > MAX_BEATS)
	{
		Logf("The chart contains too much # of beats (%d / max %d)", Logger::Severity::Warning,
			maxBeatsCount, MAX_BEATS);

		maxBeatsCount = MAX_BEATS;
	}

	for (int i = 0; i < maxBeatsCount; ++i)
	{
		const MapTime currBeat = m_beats[i] = m_beats[maxBeatsBeginInd + i];
	}

	m_beats.resize(maxBeatsCount);
}

static inline float GetSmoothValue(const float* pcm, const uint64 count, const int64 ind)
{
	const float prev = 0 <= ind - 2 && ind - 2 < 2*count ? pcm[ind - 2] : 0;
	const float curr = 0 <= ind && ind < 2*count ? pcm[ind] : 0;
	const float next = 0 <= ind + 2 && ind + 2 < 2*count ? pcm[ind + 2] : 0;

	return curr + (prev + next) * 0.5f;
}

static inline float GetAmplitude(const float* pcm, const uint64 count, const int64 sample)
{
	return std::hypotf(GetSmoothValue(pcm, count, 2*sample), GetSmoothValue(pcm, count, 2*sample + 1));
}

void OffsetComputer::ComputeEnergy()
{
	constexpr MapTime ENERGY_COUNT = COMPUTE_WINDOW + MAX_OFFSET * 2 + 10;
	constexpr float ENERGY_EPSILON = 0.000'001f;

	m_energy.clear();
	m_energy.resize(ENERGY_COUNT, 0);
	
	m_onsetScore.clear();
	m_onsetScore.resize(ENERGY_COUNT, 0);

	if (m_beats.empty())
	{
		m_energyOffset = 0;
		return;
	}

	m_energyOffset = m_beats[0] - MAX_OFFSET - 5;

	int64 ind = (static_cast<int64>(m_energyOffset) * m_sampleRate) / 1000;
	const int64 endInd = ((static_cast<int64>(m_energyOffset) + ENERGY_COUNT) * m_sampleRate) / 1000;
	int64 nextInd = (static_cast<int64>(m_energyOffset + 1) * m_sampleRate) / 1000;
	int64 intervalSize = nextInd - ind;

	int64 energyInd = 0;
	for (; ind < endInd; ++ind)
	{
		if (ind >= nextInd)
		{
			++energyInd;
			nextInd = ((static_cast<int64>(m_energyOffset) + energyInd + 1) * m_sampleRate) / 1000;
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

	bool isQuiet = true;

	// This is an experimentally-determined formula.
	for (ind = 1; ind < ENERGY_COUNT; ++ind)
	{
		const float min_energy = m_energy[ind - 1] * 0.995f;
		if (m_energy[ind] < min_energy) m_energy[ind] = min_energy;

		if (m_energy[ind] > ENERGY_EPSILON)
			isQuiet = false;

		const float prevEnergy = std::logf(std::max(m_energy[ind - 1], ENERGY_EPSILON));
		const float currEnergy = std::logf(std::max(m_energy[ind], ENERGY_EPSILON));

		m_onsetScore[ind] = (currEnergy - prevEnergy) * m_energy[ind];
	}

	if (isQuiet)
	{
		m_energy.clear();
		m_onsetScore.clear();
	}
}

int OffsetComputer::ComputeFitness(MapTime offset)
{
	int fitness = 0;
	for (MapTime beat : m_beats)
	{
		fitness += GetOnsetScore(beat + offset);
	}

	return fitness;
}

int OffsetComputer::GetOnsetScore(MapTime time)
{
	time -= m_energyOffset;

	return 0 <= time && time < static_cast<MapTime>(m_onsetScore.size()) ? Math::Clamp(static_cast<int>(100 * m_onsetScore[time]), -100, 100) : 0;
}
