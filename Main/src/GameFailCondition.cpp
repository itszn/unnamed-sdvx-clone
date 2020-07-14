#include "stdafx.h"
#include "GameFailCondition.hpp"

#include "Scoring.hpp"

bool GameFailCondition::Score::IsFailed(const Scoring& scoring) const
{
	return scoring.CalculateCurrentMaxPossibleScore() < m_score;
}

bool GameFailCondition::Grade::IsFailed(const Scoring& scoring) const
{
	const uint32 maxPossibleScore = scoring.CalculateCurrentMaxPossibleScore();
	uint32 minAllowed = 0;

	switch (m_grade)
	{
	case GradeMark::PUC:	minAllowed = 10000000; break;
	case GradeMark::S_995:	minAllowed =  9950000; break;
	case GradeMark::S:		minAllowed =  9900000; break;
	case GradeMark::AAAp:	minAllowed =  9800000; break;
	case GradeMark::AAA:	minAllowed =  9700000; break;
	case GradeMark::AAp:	minAllowed =  9500000; break;
	case GradeMark::AA:		minAllowed =  9300000; break;
	case GradeMark::Ap:		minAllowed =  9000000; break;
	case GradeMark::A:		minAllowed =  8700000; break;
	case GradeMark::B:		minAllowed =  7500000; break;
	case GradeMark::C:		minAllowed =  6500000; break;
	default: return false;
	}

	return maxPossibleScore < minAllowed;
}

bool GameFailCondition::MissCount::IsFailed(const Scoring& scoring) const
{
	return scoring.categorizedHits[0] > m_count;
}

bool GameFailCondition::MissAndNearCount::IsFailed(const Scoring& scoring) const
{
	return scoring.categorizedHits[0] + scoring.categorizedHits[1] > m_count;
}