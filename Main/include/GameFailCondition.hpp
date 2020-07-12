#pragma once

class Scoring;

class GameFailCondition
{
public:
	GameFailCondition() noexcept = default;
	virtual bool IsFailed(const Scoring&) const { return false; }

public:
	// Fails when the max possible score is lower than the given value
	class Score;
	// Fails when the max possible grade is lower than the given value
	class Grade;
	// Fails when the max possible clear mark is lower than the given value
	class ClearMark;
	// Fails when the current # of misses is above the given value
	class MissCount;
	// Fails when the current # of misses + nears is above the given value
	class MissAndNearCount;
};

class GameFailCondition::Score : public GameFailCondition
{
public:
	Score(uint32 score) noexcept : m_score(score) {}
	bool IsFailed(const Scoring&) const override;

protected:
	uint32 m_score;
};

class GameFailCondition::MissCount : public GameFailCondition
{
public:
	MissCount(uint32 count) : m_count(count) {}
	bool IsFailed(const Scoring&) const override;

protected:
	uint32 m_count;
};

class GameFailCondition::MissAndNearCount : public GameFailCondition
{
public:
	MissAndNearCount(uint32 count) : m_count(count) {}
	bool IsFailed(const Scoring&) const override;

protected:
	uint32 m_count;
};