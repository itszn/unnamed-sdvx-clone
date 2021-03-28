#pragma once
#include "stdafx.h"
#include "lua.hpp"
#include "cpr/cpr.h"

struct AsyncRequest
{
	struct lua_State* L;
	cpr::AsyncResponse r;
	int callback;
};

struct CompleteRequest
{
	struct lua_State* L;
	cpr::Response r;
	int callback;
};
