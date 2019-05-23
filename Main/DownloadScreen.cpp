#include "stdafx.h"
#include "DownloadScreen.hpp"
#include "Application.hpp"
#include "lua.hpp"
#include "archive.h"
#include "archive_entry.h"
#include "SkinHttp.hpp"
#include "cpr/util.h"

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
	m_bindable->AddFunction("Exit", this, &DownloadScreen::m_Exit);
	m_bindable->AddFunction("DownloadArchive", this, &DownloadScreen::m_DownloadArchive);
	m_bindable->AddFunction("GetSongFolder", this, &DownloadScreen::m_DownloadArchive);
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

int DownloadScreen::m_DownloadArchive(lua_State* L)
{
	String url = luaL_checkstring(L, 2);
	auto header = SkinHttp::HeaderFromLuaTable(L, 3);

	auto response = cpr::Get(cpr::Url{ url }, header);

	{
		archive* a;
		a = archive_read_new();
		archive_read_support_filter_all(a);
		archive_read_support_format_all(a);
		int r = archive_read_open_memory(a, response.text.c_str(), response.text.length());
		if (r != ARCHIVE_OK)
			return 0;

		struct archive_entry *entry;
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
			Logf("%s", Logger::Info, archive_entry_pathname(entry));
			archive_read_data_skip(a);  // Note 2
		}
		r = archive_read_free(a);
	}

	return 0;
}

int DownloadScreen::m_Exit(lua_State * L)
{
	g_application->RemoveTickable(this);
	return 0;
}
