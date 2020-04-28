#pragma once
#include "ApplicationTickable.hpp"

class DBUpdateScreen : public IApplicationTickable
{
public:
	DBUpdateScreen(int max) : m_max(max) {};
	~DBUpdateScreen() {};
	bool Init() override;
	void Tick(float deltaTime) override;
	void Render(float deltaTime) override;

	void SetCurrent(int newCurrent) { m_current = newCurrent; }
	void SetCurrent(int newCurrent, int newMax)
	{
		m_current = newCurrent;
		m_max = newMax;
	}

private:
	int m_max;
	int m_current = 0;
};
