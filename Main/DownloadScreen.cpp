#include "stdafx.h"
#include "DownloadScreen.hpp"
#include "Application.hpp"
#include "lua.hpp"

DownloadScreen::DownloadScreen()
{
}

DownloadScreen::~DownloadScreen()
{
	g_input.OnButtonPressed.RemoveAll(this);
	g_input.OnButtonReleased.RemoveAll(this);
	g_gameWindow->OnMouseScroll.RemoveAll(this);
	if(m_lua)
		g_application->DisposeLua(m_lua);
}

bool DownloadScreen::Init()
{
	m_lua = g_application->LoadScript("downloadscreen");
	if (m_lua == nullptr)
		return false;
	g_input.OnButtonPressed.Add(this, &DownloadScreen::m_OnButtonPressed);
	g_input.OnButtonReleased.Add(this, &DownloadScreen::m_OnButtonReleased);
	g_gameWindow->OnMouseScroll.Add(this, &DownloadScreen::m_OnMouseScroll);
	m_bindable = new LuaBindable(m_lua, "dlScreen");
	m_bindable->AddFunction("Exit", this, &DownloadScreen::m_exit);
	m_bindable->Push();
	lua_settop(m_lua, 0);

	return true;
}

void DownloadScreen::Tick(float deltaTime)
{
	m_advanceSong += g_input.GetInputLaserDir(1);
	int advanceSongActual = (int)Math::Floor(m_advanceSong * Math::Sign(m_advanceSong)) * Math::Sign(m_advanceSong);
	if (advanceSongActual != 0)
	{
		lua_getglobal(m_lua, "advance_selection");
		if (lua_isfunction(m_lua, -1))
		{
			lua_pushnumber(m_lua, advanceSongActual);
			if (lua_pcall(m_lua, 1, 0, 0) != 0)
			{
				Logf("Lua error on advance_selection: %s", Logger::Error, lua_tostring(m_lua, -1));
				g_gameWindow->ShowMessageBox("Lua Error on advance_selection", lua_tostring(m_lua, -1), 0);
			}
		}
		lua_settop(m_lua, 0);
	}
	m_advanceSong -= advanceSongActual;
}

void DownloadScreen::Render(float deltaTime)
{
	if (IsSuspended())
		return;

	lua_getglobal(m_lua, "render");
	lua_pushnumber(m_lua, deltaTime);
	if (lua_pcall(m_lua, 1, 0, 0) != 0)
	{
		Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
		g_gameWindow->ShowMessageBox("Lua Error", lua_tostring(m_lua, -1), 0);
		g_application->RemoveTickable(this);
	}
}

void DownloadScreen::OnKeyPressed(int32 key)
{
	lua_getglobal(m_lua, "key_pressed");
	if (lua_isfunction(m_lua, -1))
	{	
		lua_pushnumber(m_lua, key);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on key_pressed: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on key_pressed", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void DownloadScreen::OnKeyReleased(int32 key)
{
	lua_getglobal(m_lua, "key_released");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, key);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on key_released: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on key_released", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void DownloadScreen::m_OnButtonPressed(Input::Button buttonCode)
{
	lua_getglobal(m_lua, "button_pressed");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, (int32)buttonCode);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on button_pressed: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on button_pressed", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void DownloadScreen::m_OnButtonReleased(Input::Button buttonCode)
{
	lua_getglobal(m_lua, "button_released");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, (int32)buttonCode);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on button_released: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on button_released", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void DownloadScreen::m_OnMouseScroll(int32 steps)
{
	lua_getglobal(m_lua, "advance_selection");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, steps);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on advance_selection: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on advance_selection", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

int DownloadScreen::m_exit(lua_State * L)
{
	g_application->RemoveTickable(this);
	return 0;
}
