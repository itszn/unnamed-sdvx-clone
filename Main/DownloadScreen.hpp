#pragma once
#include "ApplicationTickable.hpp"
#include "Shared/LuaBindable.hpp"
#include "Input.hpp"

class DownloadScreen : public IApplicationTickable
{
public:
	DownloadScreen();
	~DownloadScreen();
	bool Init() override;
	// Tick for tickable
	void Tick(float deltaTime) override;
	void Render(float deltaTime) override;

	void OnKeyPressed(int32 key) override;
	void OnKeyReleased(int32 key) override;
private:
	struct lua_State* m_lua;
	LuaBindable* m_bindable;
	float m_advanceSong = 0.0f;

	void m_OnButtonPressed(Input::Button buttonCode);
	void m_OnButtonReleased(Input::Button buttonCode);
	void m_OnMouseScroll(int32 steps);
	int m_Exit(struct lua_State* L);
	int m_DownloadArchive(struct lua_State* L);
};