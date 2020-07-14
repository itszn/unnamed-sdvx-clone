#pragma once

enum class GradeMark : uint8
{
	D, C, B, A, Ap, AA, AAp, AAA, AAAp, S,
	S_995, PUC
};

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