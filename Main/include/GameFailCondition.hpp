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
	// Failes when the gauge is less than the given value
	class Gauge;
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

	uint32 GetMinAllowed() const { return m_score; }

protected:
	uint32 m_score;
};

class GameFailCondition::Grade : public GameFailCondition
{
public:
	Grade(GradeMark grade) noexcept : m_grade(grade) {}
	bool IsFailed(const Scoring&) const override;

	GradeMark GetMinAllowed() const { return m_grade; }

protected:
	GradeMark m_grade;
};

class GameFailCondition::Gauge : public GameFailCondition
{
public:
	Gauge(float gauge) noexcept : m_gauge(gauge) {}
	bool IsFailed(const Scoring&) const override;

	float GetMinAllowed() const { return m_gauge; }

protected:
	float m_gauge;
};

class GameFailCondition::MissCount : public GameFailCondition
{
public:
	MissCount(uint32 count) : m_count(count) {}
	bool IsFailed(const Scoring&) const override;

	uint32 GetMaxAllowed() const { return m_count; }

protected:
	uint32 m_count;
};

class GameFailCondition::MissAndNearCount : public GameFailCondition
{
public:
	MissAndNearCount(uint32 count) : m_count(count) {}
	bool IsFailed(const Scoring&) const override;

	uint32 GetMaxAllowed() const { return m_count; }

protected:
	uint32 m_count;
};