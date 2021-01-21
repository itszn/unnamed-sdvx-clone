#pragma once
#include "stdafx.h"
#include <Beatmap/BeatmapObjects.hpp>
#include "Scoring.hpp"

class Gauge {
public:
	Gauge() = default;
	virtual ~Gauge() = default;
	virtual bool Init(MapTotals mapTotals, uint16 total, MapTime length) = 0;
	virtual void LongHit() = 0;	
	virtual void CritHit() = 0;
	virtual void NearHit() = 0;
	virtual void LongMiss() = 0;
	virtual void ShortMiss() = 0;
	virtual void SetValue(float v) {
		m_gauge = v;
	}
	virtual float GetValue() const {
		return m_gauge;
	};
	virtual const std::array<float, 256>& GetSamples() const {
		return m_samples;
	}

	virtual void Update(MapTime currentTime);


	virtual bool GetClearState() const = 0;
	virtual const char* GetName() const = 0;

	// Returns true if the gauge should fail out the player
	virtual bool FailOut() const {
		return false;
	};
protected:
	virtual void InitSamples(MapTime length);

	std::array<float, 256> m_samples;
	float m_gauge = 0.0f;
	MapTime m_sampleDuration = 1;

};

class GaugeNormal : public Gauge {
public:
	GaugeNormal() = default;
	~GaugeNormal() = default;
	bool Init(MapTotals mapTotals, uint16 total, MapTime length);
	void LongHit();
	void CritHit();
	void NearHit();
	void LongMiss();
	void ShortMiss();
	bool GetClearState() const;
	const char* GetName() const;


private:
	float m_shortMissDrain;
	float m_drainMultiplier;
	float m_shortGaugeGain;
	float m_tickGaugeGain;
};

class GaugeHard : public Gauge {
public:
	GaugeHard() = default;
	~GaugeHard() = default;
	bool Init(MapTotals mapTotals, uint16 total, MapTime length);
	void LongHit();
	void CritHit();
	void NearHit();
	void LongMiss();
	void ShortMiss();
	bool GetClearState() const;
	const char* GetName() const;
	bool FailOut() const;

private:
	float DrainMultiplier() const;

	float m_shortMissDrain;
	float m_drainMultiplier;
	float m_shortGaugeGain;
	float m_tickGaugeGain;
};

