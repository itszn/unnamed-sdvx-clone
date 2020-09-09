#pragma once

#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"
#include <Beatmap/MapDatabase.hpp>

template<typename T>
class Optional {
protected:
	bool m_hasValue = false;
	T m_val;
public:
	Optional() : m_hasValue(false) {}
	Optional(T v) : m_val(v), m_hasValue(true) {}
	bool HasValue() const { return m_hasValue; }
	T operator*() const
	{
		assert(m_hasValue);
		return m_val;
	}
	void Reset()
	{
		m_hasValue = false;
	}
};

template<typename T>
class ChallengeOption : public Optional<T> {
protected:
	bool m_isNull;
public:
	ChallengeOption() : Optional(), m_isNull(false) {}
	ChallengeOption(T v) : Optional(v), m_isNull(false) {}
	bool IsNull() const { return m_isNull; }
	void Reset()
	{
		Optional<T>::Reset();
		m_isNull = false;
	}
	ChallengeOption Merge(const ChallengeOption& overrider) const
	{
		if (overrider.HasValue() || overrider.IsNull())
			return overrider;
		return *this;
	}

	// This will pass if not overriden
	static ChallengeOption<T> IgnoreOption() {
		return ChallengeOption<T>();
	}
	// This will force an overriden to pass
	static ChallengeOption<T> DisableOption() {
		auto v = ChallengeOption<T>();
		v.m_isNull = true;
		return v;
	}
};

// This class add a m_passed value, which is true if the requirement
// was passed by the user (including a null requirement)
template<typename T>
class ChallengeRequirement : public ChallengeOption<T> {
protected:
	bool m_passed = false;
public:
	ChallengeRequirement() : ChallengeOption(), m_passed(false) {}
	ChallengeRequirement(T v) : ChallengeOption(v), m_passed(false) {}
	ChallengeRequirement(const ChallengeOption<T>& o) : ChallengeOption(o), m_passed(false) {}
	// This will be true if marked as passed or no value
	bool PassedOrIgnored() const { return !HasValue() || m_passed; }
	// This will be true if marked as passed or null
	bool PassedOrNull() const { return m_passed || IsNull(); }
	void MarkPassed() { m_passed = true; }
	void Reset()
	{
		ChallengeOption<T>::Reset();
		m_passed = false;
	}
};

// NOTE: macros are used here to reduce the amount of copy paste required to update
//       or add new options
struct ChallengeOptions{
#define CHALLENGE_OPTIONS_ALL(v) \
	v(bool, mirror) \
	v(bool, excessive) \
	v(uint32, min_modspeed) \
	v(uint32, max_modspeed) \
	v(float, hidden_min) \
	v(float, hidden_max) \
	v(uint32, crit_judge) \
	v(uint32, near_judge) \
	v(bool, allow_cmod)

#define CHALLENGE_OPTION_DEC(t, n) ChallengeOption<t> n;
	CHALLENGE_OPTIONS_ALL(CHALLENGE_OPTION_DEC);
#undef CHALLENGE_OPTION_DEC

	ChallengeOptions Merge(const ChallengeOptions& overrider)
	{
		ChallengeOptions out;
#define CHALLENGE_OPTIONS_MERGE(t, n) out. ##n = n.Merge(overrider. ##n);
		CHALLENGE_OPTIONS_ALL(CHALLENGE_OPTIONS_MERGE)
#undef CHALLENGE_OPTIONS_MERGE
	}
#undef CHALLENGE_OPTIONS_ALL
};

struct ChallengeRequirements
{
	bool evaluated = false;
#define CHALLENGE_REQS_ALL(v) \
	v(bool, clear) \
	v(uint32, min_score) \
	v(float, min_gauge) \
	v(uint32, min_errors) \
	v(uint32, min_nears) \
	v(uint32, min_chain)
#define CHALLENGE_REQ_DEC(t, n) ChallengeRequirement<t> n;
	CHALLENGE_REQS_ALL(CHALLENGE_REQ_DEC);
#undef CHALLENGE_REQ_DEC
	// This takes a second evaluated reqs which override this one
	// if the second set doesn't have an active member, this one is used
	bool Passed(struct ChallengeRequirements& over)
	{
		assert(evaluated && over.evaluated);
#define CHALLENGE_REQ_PASSED_OVERRIDE(t, n) res = res && (over. ##n.PassedOrNull() || n.PassedOrIgnored());
		bool res = true;
		CHALLENGE_REQS_ALL(CHALLENGE_REQ_PASSED_OVERRIDE);
		return res;
#undef CHALLENGE_REQ_PASSED_OVERRIDE
	}
	bool Passed()
	{
		assert(evaluated);
#define CHALLENGE_REQ_PASSED(t, n) res = res && (n.PassedOrIgnored());
		bool res = true;
		CHALLENGE_REQS_ALL(CHALLENGE_REQ_PASSED);
		return res;
#undef CHALLENGE_REQ_PASSED
	}
	void Reset()
	{
		evaluated = false;
#define CHALLENGE_REQ_RESET(t, n) n.Reset();
		CHALLENGE_REQS_ALL(CHALLENGE_REQ_RESET);
#undef CHALLENGE_REQ_RESET
	}
};

class ChallengeManager
{
private:
	ChallengeIndex* m_chal = nullptr;
	ChartIndex* m_currentChart = nullptr;
	int m_chartIndex = 0;
	bool m_running = false;
	Vector<uint32> m_scores;
	ChallengeRequirements m_globalReqs;
	Vector<ChallengeRequirements> m_reqs;
public:
	bool RunningChallenge() { return m_running; }
	bool StartChallenge(ChallengeIndex* chal);
	friend class Game;
	void ReportScore(Game*, ClearMark);

private:
	ChallengeOption<uint32> m_getOptionAsPositiveInteger(
		nlohmann::json reqs, String name, uint64 min=0, uint64 max=INT_MAX);

	ChallengeOption<float> m_getOptionAsFloat(
		nlohmann::json reqs, String name, float min=-INFINITY, float max=INFINITY);

	ChallengeOption<bool> m_getOptionAsBool(
		nlohmann::json reqs, String name);

	ChallengeRequirements m_processReqs(nlohmann::json req);
	ChallengeOptions m_processOptions(nlohmann::json req);
};

struct ChallengeSelectIndex
{
private:
	ChallengeIndex* m_challenge;
	// TODO folder?
public:
	ChallengeSelectIndex() = default;
	ChallengeSelectIndex(ChallengeIndex* chal)
		: m_challenge(chal), id(chal->id)
	{
	}

	int32 id;
	ChallengeIndex* GetChallenge() const { return m_challenge; }
};

class ChallengeSelect : public IAsyncLoadableApplicationTickable
{
protected:
	ChallengeSelect() = default;
public:
	virtual ~ChallengeSelect() = default;
	static ChallengeSelect* Create();

	virtual ChallengeIndex* GetCurrentSelectedChallenge() { return 0; }
};
