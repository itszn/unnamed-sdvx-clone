#pragma once
#include "ApplicationTickable.hpp"
#include "GameConfig.hpp"
#include "SkinConfig.hpp"

class SettingsScreen : public IApplicationTickable
{
protected:
	SettingsScreen() = default;
public:
	virtual ~SettingsScreen() = default;
	static SettingsScreen* Create();
	static void NKRender();
};

class ButtonBindingScreen : public IApplicationTickable
{
protected:
	ButtonBindingScreen() = default;
public:
	virtual ~ButtonBindingScreen() = default;
	static ButtonBindingScreen* Create(GameConfigKeys key, bool gamepad = false, int controllerIndex = 0);
};

class LaserSensCalibrationScreen : public IApplicationTickable
{
protected:
	LaserSensCalibrationScreen() = default;
public:
	virtual ~LaserSensCalibrationScreen() = default;
	static LaserSensCalibrationScreen* Create();
	Delegate<float> SensSet;
};

class SkinSettingsScreen : public IApplicationTickable
{
public:
	SkinSettingsScreen(String skin, struct nk_context* ctx);
	~SkinSettingsScreen();
	void Tick(float deltatime) override;
	void Render(float deltaTime) override;
	bool Init() override;
private:
	bool ToggleSetting(String key, String label);
	bool PercentSetting(String key, String label);
	bool TextSetting(String key, String label, bool secret);
	bool ColorSetting(String key, String label);
	bool FloatSetting(String key, String label, float min, float max, float step = 0.01);
	bool IntSetting(String key, String label, int min, int max, int step = 1, int perpixel = 1);
	bool StringSelectionSetting(String key, String label, SkinSetting& setting);
	bool MainConfigStringSelectionSetting(GameConfigKeys key, Vector<String> options, String label);
	void Exit();

	class SkinConfig* m_skinConfig;
	struct nk_context* m_nctx;
	String m_skin;
	Map<String, bool> m_hsvMap;
	Vector<String> m_allSkins;
};