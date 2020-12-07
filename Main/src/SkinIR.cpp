#include "stdafx.h"
#include "Shared/LuaBindable.hpp"
#include "SkinIR.hpp"
#include "IR.hpp"

//this file mostly based on SkinHttp
//but it seems easier to me to have these be separate functions rather than updating the http requests to support json
//especially since that'd then require the skin to specify the authorization headers, json Content-Type, etc.
//this approach makes it easier for the skin creator

void SkinIR::m_requestLoop()
{
	while (m_running)
	{
		m_mutex.lock(); ///TODO: use semaphore?
		if (m_requests.size() > 0)
		{
			//get request
			AsyncRequest* r = m_requests.front();
			m_mutex.unlock();

			//process request
			CompleteRequest cr;
			cr.L = r->L;
			cr.callback = r->callback;
			cr.r = r->r.get();

			//push result and pop request
			m_mutex.lock();
			delete r;
			m_callbackQueue.push(cr);
			m_requests.pop();
			m_mutex.unlock();
		}
		else
		{
			m_mutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}

//TODO: change
void SkinIR::m_PushResponse(lua_State * L, const cpr::Response & r)
{
	auto pushString = [L](String key, String value)
	{
		lua_pushstring(L, *key);
		lua_pushstring(L, *value);
		lua_settable(L, -3);
	};
	auto pushNumber = [L](String key, double value)
	{
		lua_pushstring(L, *key);
		lua_pushnumber(L, value);
		lua_settable(L, -3);
	};

	lua_newtable(L);
	pushString("url", r.url);
	pushString("text", r.text);
	pushNumber("status", r.status_code);
	pushNumber("elapsed", r.elapsed);
	pushString("error", r.error.message.c_str());
	pushString("cookies", r.cookies.GetEncoded().c_str());
	lua_pushstring(L, "header");
	lua_newtable(L);
	for (auto& i : r.header)
	{
		pushString(i.first, i.second);
	}
	lua_settable(L, -3);

}


SkinIR::SkinIR()
{
	m_running = true;
	m_requestThread = Thread(&SkinIR::m_requestLoop, this);
}

SkinIR::~SkinIR()
{
	m_running = false;
	if(m_requestThread.joinable())
		m_requestThread.join();

	while (!m_requests.empty())
	{
		delete m_requests.front();
		m_requests.pop();
	}

	for (auto& s : m_boundStates)
	{
		delete s.second;
	}
	m_boundStates.clear();
}

int SkinIR::lHeartbeat(struct lua_State* L)
{
    int callback = luaL_ref(L, LUA_REGISTRYINDEX);
    AsyncRequest* r = new AsyncRequest();
    r->r = IR::Heartbeat();
    r->callback = callback;
    r->L = L;
    m_mutex.lock();
    m_requests.push(r);
    m_mutex.unlock();
    return 0;
}

int SkinIR::lChartTracked(struct lua_State* L)
{
    //todo
    return 0;
}


void SkinIR::ProcessCallbacks()
{
	m_mutex.lock();
	if (m_callbackQueue.size() > 0)
	{
		//get response
		CompleteRequest& cr = m_callbackQueue.front();
		m_mutex.unlock();

		if (m_boundStates.Contains(cr.L))
		{
			//process response
			lua_rawgeti(cr.L, LUA_REGISTRYINDEX, cr.callback);
			m_PushResponse(cr.L, cr.r);
			if (lua_pcall(cr.L, 1, 0, 0) != 0)
			{
				Logf("Lua error on calling IR callback: %s", Logger::Severity::Error, lua_tostring(cr.L, -1));
			}
			lua_settop(cr.L, 0);
			luaL_unref(cr.L, LUA_REGISTRYINDEX, cr.callback);
		}
		//pop response
		m_mutex.lock();
		m_callbackQueue.pop();
	}
	m_mutex.unlock();
}

void SkinIR::PushFunctions(lua_State * L)
{
	auto bindable = new LuaBindable(L, "IR");
	bindable->AddFunction("Heartbeat", this, &SkinIR::lHeartbeat);
	bindable->Push();
	lua_settop(L, 0);
	m_boundStates.Add(L, bindable);
}

void SkinIR::ClearState(lua_State * L)
{
	if (!m_boundStates.Contains(L))
		return;
	delete m_boundStates.at(L);
	m_boundStates.erase(L);
}
