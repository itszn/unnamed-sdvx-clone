#include "stdafx.h"
#include "GameFailCondition.hpp"

#include "Scoring.hpp"

bool GameFailCondition::Score::IsFailed(const Scoring& scoring) const
{
	return scoring.CalculateCurrentMaxPossibleScore() < m_score;
}

bool GameFailCondition::Grade::IsFailed(const Scoring& scoring) const
{
	return scoring.CalculateCurrentMaxPossibleScore() < ToMinScore(m_grade);
}

bool GameFailCondition::MissCount::IsFailed(const Scoring& scoring) const
{
	return scoring.categorizedHits[0] > m_count;
}

bool GameFailCondition::MissAndNearCount::IsFailed(const Scoring& scoring) const
{
	return scoring.categorizedHits[0] + scoring.categorizedHits[1] > m_count;
}