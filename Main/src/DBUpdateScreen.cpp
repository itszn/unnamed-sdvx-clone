#include "stdafx.h"
#include "DBUpdateScreen.hpp"
#include "Application.hpp"

bool DBUpdateScreen::Init()
{
	return true;
}

void DBUpdateScreen::Tick(float deltaTime)
{

}

void DBUpdateScreen::Render(float deltaTime)
{
	g_application->FastText("Updating DB... Please wait...", 10, 10, 32, 0);
	g_application->FastText(Utility::Sprintf("%u / %u", m_current, m_max), 10, 50, 32, 0);
}
