#pragma once
#include "ApplicationTickable.hpp"

enum class GameConfigKeys : uint32;

class SettingsScreen
{
private:
	SettingsScreen() = delete;
	~SettingsScreen() = delete;

public:
	static IApplicationTickable* Create();
};

class ButtonBindingScreen : public IApplicationTickable
{
protected:
	ButtonBindingScreen() = default;
public:
	virtual ~ButtonBindingScreen() = default;
	static ButtonBindingScreen* Create(GameConfigKeys key, bool gamepad = false, int controllerIndex = 0, bool isAlternative = false);
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
