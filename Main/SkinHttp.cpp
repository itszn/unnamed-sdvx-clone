#include "stdafx.h"
#include "SkinHttp.hpp"
#include "lua.hpp"
#include "Shared/LuaBindable.hpp"

void SkinHttp::m_requestLoop()
{
	while (m_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

SkinHttp::SkinHttp()
{
	m_running = true;
	m_requestThread = Thread(&SkinHttp::m_requestLoop, this);
}

SkinHttp::~SkinHttp()
{
	m_running = false;
	if(m_requestThread.joinable())
		m_requestThread.join();

	m_requests.clear();
	m_callbackQueue.clear();
}

int SkinHttp::lGetAsync(lua_State * L)
{
	return 0;
}

int SkinHttp::lPostAsync(lua_State * L)
{
	return 0;
}

int SkinHttp::lGet(lua_State * L)
{
	return 0;
}

int SkinHttp::lPost(lua_State * L)
{
	return 0;
}

void SkinHttp::ProcessCallbacks()
{
}

void SkinHttp::PushFunctions(lua_State * L)
{
	auto bindable = new LuaBindable(L, "Http");
	bindable->AddFunction("Get", this, &SkinHttp::lGet);
	bindable->AddFunction("Post", this, &SkinHttp::lPost);
	bindable->AddFunction("GetAsync", this, &SkinHttp::lGetAsync);
	bindable->AddFunction("PostAsync", this, &SkinHttp::lPostAsync);
	bindable->Push();
	lua_settop(L, 0);
	m_boundStates.Add(L, bindable);
}

void SkinHttp::ClearState(lua_State * L)
{
	if (!m_boundStates.Contains(L))
		return;
	delete m_boundStates.at(L);
	m_boundStates.erase(L);
}
