#pragma once
#include "ApplicationTickable.hpp"
#include "Shared/LuaBindable.hpp"
#include "Input.hpp"
#include "Shared/Thread.hpp"
#include "cpr/cpr.h"
#include <queue>
#include "Beatmap/MapDatabase.hpp"
#include "json.hpp"
#include "TCPSocket.hpp"
#include <Scoring.hpp>

class MultiplayerScreen : public IApplicationTickable
{
public:
	MultiplayerScreen();
	~MultiplayerScreen();

	bool Init() override;
	void Tick(float deltaTime) override;
	void Render(float deltaTime) override;

	void OnKeyPressed(int32 key) override;
	void OnKeyReleased(int32 key) override;
	void MousePressed(MouseButton button);

	virtual void OnSuspend();
	virtual void OnRestore();

	int lExit(struct lua_State* L);
	int lSongSelect(struct lua_State* L);

	void SetSelectedMap(MapIndex*, DifficultyIndex*);

	void PerformScoreTick(Scoring& scoring);
	void SendFinalScore(Scoring& scoring, int clearState);

	TCPSocket& GetTCP()
	{
		return m_tcp;
	}

	String GetUserId()
	{
		return m_userId;
	}

private:
	void m_OnButtonPressed(Input::Button buttonCode);
	void m_OnButtonReleased(Input::Button buttonCode);
	void m_OnMouseScroll(int32 steps);

	bool m_handleStartPacket(nlohmann::json& packet);
	bool m_handleAuthResponse(nlohmann::json& packet);
	bool m_handleSongChange(nlohmann::json& packet);
	void m_handleSocketClose();

	void m_updateSelectedMap(int32 mapid, int32 diff_ind, bool is_new);
	void m_clearLuaMap();
	DifficultyIndex* m_getMapByShortPath(const String& path, int32);

	void m_change_difficulty(int offset);

	// Some lua util functions
	void m_PushStringToTable(const char* name, const char* data)
	{
		lua_pushstring(m_lua, name);
		lua_pushstring(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushFloatToTable(const char* name, float data)
	{
		lua_pushstring(m_lua, name);
		lua_pushnumber(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushIntToTable(const char* name, int data)
	{
		lua_pushstring(m_lua, name);
		lua_pushinteger(m_lua, data);
		lua_settable(m_lua, -3);
	}

	struct lua_State* m_lua = nullptr;
	LuaBindable* m_bindable = nullptr;
	bool m_suspended = false;



	uint32 m_lastScoreSent = 0;
	// TODO(itszn) have the server adjust this
	uint32 m_scoreInterval = 200;

	// Unique id given to by the server on auth
	String m_userId;

	// Socket to talk to the server
	TCPSocket m_tcp;

	// Database ids of the selected map
	int32 m_selectedMapId = 0;
	String m_selectedMapShortPath;

	// Selected index into the map's difficulties
	int32 m_selectedDiffIndex = 0;
	bool m_hasSelectedMap = false;

	// Instance of the database, used to look up songs
	MapDatabase m_mapDatabase;
};