#pragma once
#include "AudioOutput.hpp"
#include "AudioBase.hpp"

// Threading
#include <thread>
#include <mutex>
using std::thread;
using std::mutex;

class Audio_Impl : public IMixer
{
public:
	void Start();
	void Stop();
	// Get samples
	virtual void Mix(void* data, uint32& numSamples) override;
	// Registers an AudioBase to be rendered
	void Register(AudioBase* audio);
	// Removes an AudioBase so it is no longer rendered
	void Deregister(AudioBase* audio);

	uint32 GetSampleRate() const;
	double GetSecondsPerSample() const;

	float globalVolume = 1.0f;

	mutex lock;
	Vector<AudioBase*> itemsToRender;
	Vector<DSP*> globalDSPs;

	class LimiterDSP* limiter = nullptr;

	// Used to limit rendering to a fixed number of samples (512)
	float* m_sampleBuffer = nullptr;
	uint32 m_sampleBufferLength = 384;
	uint32 m_remainingSamples = 0;
	bool m_isMuted = false;

	thread audioThread;
	bool runAudioThread = false;
	AudioOutput* output = nullptr;
};