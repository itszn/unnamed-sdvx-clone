#include "stdafx.h"
#include "GameFailCondition.hpp"

#include "Scoring.hpp"

const char* GameFailCondition::TYPE_STR[6] = { "None", "Score", "Grade", "Miss", "MissAndNear", "Gauge" };

bool GameFailCondition::Score::IsFailed(const Scoring& scoring) const
{
	return scoring.CalculateCurrentMaxPossibleScore() < m_score;
}

bool GameFailCondition::Grade::IsFailed(const Scoring& scoring) const
{
	return scoring.CalculateCurrentMaxPossibleScore() < ToMinScore(m_grade);
}

bool GameFailCondition::Gauge::IsFailed(const Scoring& scoring) const
{
	if (scoring.IsPerfect()) return false;

	if (m_gauge == 0) return false;
	if (m_gauge == 100) return scoring.currentGauge < 1.0f;

	return scoring.currentGauge * 100 < m_gauge;
}

bool GameFailCondition::MissCount::IsFailed(const Scoring& scoring) const
{
	return scoring.categorizedHits[0] > m_count;
}

bool GameFailCondition::MissAndNearCount::IsFailed(const Scoring& scoring) const
{
	return scoring.categorizedHits[0] + scoring.categorizedHits[1] > m_count;
}
