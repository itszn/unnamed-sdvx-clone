#pragma once

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
