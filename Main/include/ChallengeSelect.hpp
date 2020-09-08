#pragma once

#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"
#include <Beatmap/MapDatabase.hpp>

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
