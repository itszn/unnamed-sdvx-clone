#pragma once
#include "stdafx.h"
#include "Shared/Thread.hpp"
#include "cpr/cpr.h"


struct AsyncRequest
{
	struct lua_State* L;
	std::future<cpr::Response> r;
	int callback;
};

struct CompleteRequest
{
	struct lua_State* L;
	cpr::Response r;
	int callback;
};

class SkinHttp
{
public:
	SkinHttp();
	~SkinHttp();
	int lGetAsync(struct lua_State* L);
	int lPostAsync(struct lua_State* L);
	int lGet(struct lua_State* L);
	int lPost(struct lua_State* L);
	void ProcessCallbacks();
	void PushFunctions(struct lua_State* L);
	void ClearState(struct lua_State* L);

private:
	Thread m_requestThread;
	Mutex m_mutex;
	Vector<AsyncRequest> m_requests;
	Vector<CompleteRequest> m_callbackQueue;
	bool m_running;
	void m_requestLoop();
	Map<struct lua_State*, class LuaBindable*> m_boundStates;
};