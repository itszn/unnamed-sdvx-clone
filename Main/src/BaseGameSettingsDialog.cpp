#include "stdafx.h"
#include "BaseGameSettingsDialog.hpp"

#include "lua.hpp"
#include "Application.hpp"

BaseGameSettingsDialog::~BaseGameSettingsDialog()
{
	if (m_lua)
	{
		g_application->DisposeLua(m_lua);
		m_lua = nullptr;
	}

	for (auto& tab : m_tabs)
	{
		tab->settings.clear();
	}

	m_tabs.clear();

	g_input.OnButtonReleased.RemoveAll(this);
	g_input.OnButtonPressed.RemoveAll(this);
	g_gameWindow->OnKeyPressed.RemoveAll(this);
}

void BaseGameSettingsDialog::Tick(float deltaTime)
{
    if (m_active != m_targetActive)
    {
        m_active = m_targetActive;
        if (!m_active)
        {
            onClose.Call(this);
        }
    }

    if (!m_active)
    {
        return;
    }

    if (!m_enableFXInputs)
    {
        m_enableFXInputs = !g_input.GetButton(Input::Button::FX_0) && !g_input.GetButton(Input::Button::FX_1);
    }

    //tick inputs
    for (size_t i = 0; i < 2; i++)
    {
        m_knobAdvance[i] += g_input.GetInputLaserDir(i) * m_sensMult;
    }

    m_AdvanceSelection(m_knobAdvance[0]);
    m_knobAdvance[0] -= truncf(m_knobAdvance[0]);

    if (m_currentSetting >= m_tabs[m_currentTab]->settings.size())
        return;

    auto currentSetting = m_tabs[m_currentTab]->settings.at(m_currentSetting).get();
    if (currentSetting->type == SettingType::Floating)
    {
        float advance = m_knobAdvance[1] * currentSetting->floatSetting.mult;
        if (g_input.GetButton(Input::Button::BT_0))
            advance /= 4.0f;
        if (g_input.GetButton(Input::Button::BT_1))
            advance /= 2.0f;
        if (g_input.GetButton(Input::Button::BT_2))
            advance *= 2.0f;
        if (g_input.GetButton(Input::Button::BT_3))
            advance *= 4.0f;
        currentSetting->floatSetting.val += advance;
        currentSetting->floatSetting.val = Math::Clamp(currentSetting->floatSetting.val,
            currentSetting->floatSetting.min,
            currentSetting->floatSetting.max);
        currentSetting->floatSetting.setter.Call(currentSetting->floatSetting.val);
        m_knobAdvance[1] = 0.f;
    }
    else
    {
        m_ChangeStepSetting((int)m_knobAdvance[1]);
        m_knobAdvance[1] -= truncf(m_knobAdvance[1]);
    }
}

void BaseGameSettingsDialog::Render(float deltaTime)
{
    m_SetTables();
    lua_getglobal(m_lua, "render");
    lua_pushnumber(m_lua, deltaTime);
    lua_pushboolean(m_lua, m_active);
    if (lua_pcall(m_lua, 2, 0, 0) != 0)
    {
        Logf("Lua error: %s", Logger::Severity::Error, lua_tostring(m_lua, -1));
        g_gameWindow->ShowMessageBox("Lua Error", lua_tostring(m_lua, -1), 0);
        assert(false);
    }
    lua_settop(m_lua, 0);
}

bool BaseGameSettingsDialog::Init()
{
    m_tabs.clear();
    m_currentTab = 0;

    m_lua = g_application->LoadScript("gamesettingsdialog");
    if (!m_lua)
    {
        bool copyDefault = g_gameWindow->ShowYesNoMessage("Missing game settings dialog", "No game settings dialog script file could be found, suggested solution:\n"
            "Would you like to copy \"scripts/gamesettingsdialog.lua\" from the default skin to your current skin?");
        if (!copyDefault)
            return false;

        String defaultPath = Path::Normalize(Path::Absolute("skins/Default/scripts/gamesettingsdialog.lua"));
        String skinPath = Path::Normalize(Path::Absolute("skins/" + g_application->GetCurrentSkin() + "/scripts/gamesettingsdialog.lua"));
        Path::Copy(defaultPath, skinPath);

        m_lua = g_application->LoadScript("gamesettingsdialog");
        if (!m_lua)
        {
            g_gameWindow->ShowMessageBox("Missing sort selection", "No sort selection script file could be found and the system was not able to copy the default", 2);
            return false;
        }
    }

    InitTabs();

    m_SetTables();
    g_input.OnButtonPressed.Add(this, &BaseGameSettingsDialog::m_OnButtonPressed);
    g_input.OnButtonReleased.Add(this, &BaseGameSettingsDialog::m_OnButtonReleased);
    g_gameWindow->OnKeyPressed.Add(this, &BaseGameSettingsDialog::m_OnKeyPressed);

    m_isInitialized = true;

    return true;
}

BaseGameSettingsDialog::Setting BaseGameSettingsDialog::CreateFloatSetting(GameConfigKeys key, String name, Vector2 range, float mult)
{
    Setting s = std::make_unique<SettingData>(SettingData({
        name,
        SettingType::Floating,
        }));

    auto getter = [key](float& value) {
        value = g_gameConfig.GetFloat(key);
    };

    auto setter = [key](float newValue) {
        g_gameConfig.Set(key, newValue);
    };

    s->floatSetting.val = g_gameConfig.GetFloat(key);
    s->floatSetting.min = range.x;
    s->floatSetting.max = range.y;
    s->floatSetting.mult = mult;
    s->floatSetting.setter.AddLambda(std::move(setter));
    s->floatSetting.getter.AddLambda(std::move(getter));
    return s;
}

BaseGameSettingsDialog::Setting BaseGameSettingsDialog::CreateIntSetting(GameConfigKeys key, String name, Vector2i range)
{
    Setting s = std::make_unique<SettingData>(SettingData({
        name,
        SettingType::Integer,
        }));

    auto getter = [key](int& value) {
        value = g_gameConfig.GetInt(key);
    };

    auto setter = [key](int newValue) {
        g_gameConfig.Set(key, newValue);
    };

    s->intSetting.val = g_gameConfig.GetInt(key);
    s->intSetting.min = range.x;
    s->intSetting.max = range.y;
    s->intSetting.step = 1;
    s->intSetting.setter.AddLambda(std::move(setter));
    s->intSetting.getter.AddLambda(std::move(getter));
    return s;
}

BaseGameSettingsDialog::Setting BaseGameSettingsDialog::CreateToggleSetting(GameConfigKeys key, String name)
{
    Setting s = std::make_unique<SettingData>(SettingData({
        name,
        SettingType::Toggle,
        }));


    auto getter = [key](bool& value) {
        value = g_gameConfig.GetBool(key);
    };

    auto setter = [key](bool newValue) {
        g_gameConfig.Set(key, newValue);
    };

    s->boolSetting.val = g_gameConfig.GetBool(key);
    s->boolSetting.setter.AddLambda(std::move(setter));
    s->boolSetting.getter.AddLambda(std::move(getter));
    return s;
}

void BaseGameSettingsDialog::m_SetTables()
{
    assert(m_lua);

    auto pushStringToTable = [&](const char* name, String data) {
        lua_pushstring(m_lua, name);
        lua_pushstring(m_lua, data.c_str());
        lua_settable(m_lua, -3);
    };
    auto pushIntToTable = [&](const char* name, int data) {
        lua_pushstring(m_lua, name);
        lua_pushinteger(m_lua, data);
        lua_settable(m_lua, -3);
    };
    auto pushBoolToTable = [&](const char* name, bool data) {
        lua_pushstring(m_lua, name);
        lua_pushboolean(m_lua, data);
        lua_settable(m_lua, -3);
    };
    auto pushFloatToTable = [&](const char* name, float data) {
        lua_pushstring(m_lua, name);
        lua_pushnumber(m_lua, data);
        lua_settable(m_lua, -3);
    };

    lua_newtable(m_lua);
    {
        lua_pushstring(m_lua, "tabs");
        lua_newtable(m_lua);
        {
            int tabCounter = 0;
            for (auto&& tab : m_tabs)
            {
                lua_pushinteger(m_lua, ++tabCounter);
                lua_newtable(m_lua);
                pushStringToTable("name", tab->name);
                lua_pushstring(m_lua, "settings");
                lua_newtable(m_lua);
                {
                    int settingCounter = 0;
                    for (auto&& setting : tab->settings)
                    {
                        lua_pushinteger(m_lua, ++settingCounter);
                        lua_newtable(m_lua);
                        pushStringToTable("name", setting->name);

                        //also get settings values in case they've been changed elsewhere
                        String newEnumVal;
                        switch (setting->type)
                        {
                        case SettingType::Integer:
                            setting->intSetting.getter.Call(setting->intSetting.val);
                            pushStringToTable("type", "int");
                            pushIntToTable("value", setting->intSetting.val);
                            pushIntToTable("min", setting->intSetting.min);
                            pushIntToTable("max", setting->intSetting.max);
                            break;
                        case SettingType::Floating:
                            setting->floatSetting.getter.Call(setting->floatSetting.val);
                            pushStringToTable("type", "float");
                            pushFloatToTable("value", setting->floatSetting.val);
                            pushIntToTable("min", setting->floatSetting.min);
                            pushIntToTable("max", setting->floatSetting.max);
                            break;
                        case SettingType::Toggle:
                        case SettingType::Button:
                            setting->boolSetting.getter.Call(setting->boolSetting.val);
                            pushStringToTable("type", "toggle");
                            pushBoolToTable("value", setting->type == SettingType::Button ? false : setting->boolSetting.val);
                            break;
                        case SettingType::Enum:
                            setting->enumSetting.getter.Call(newEnumVal);
                            setting->enumSetting.val = std::distance(setting->enumSetting.options.begin(),
                                std::find(setting->enumSetting.options.begin(),
                                    setting->enumSetting.options.end(),
                                    newEnumVal));
                            pushStringToTable("type", "enum");
                            pushIntToTable("value", setting->enumSetting.val + 1);
                            lua_pushstring(m_lua, "options");
                            lua_newtable(m_lua);
                            int optionCounter = 0;
                            for (auto&& o : setting->enumSetting.options)
                            {
                                lua_pushinteger(m_lua, ++optionCounter);
                                lua_pushstring(m_lua, *o);
                                lua_settable(m_lua, -3);
                            }
                            lua_settable(m_lua, -3);
                            break;
                        }
                        lua_settable(m_lua, -3);
                    }
                    lua_settable(m_lua, -3);
                }
                lua_settable(m_lua, -3);
            }
            lua_settable(m_lua, -3);
        }

        pushIntToTable("currentTab", m_currentTab + 1);
        pushIntToTable("currentSetting", m_currentSetting + 1);
    }
    lua_setglobal(m_lua, "SettingsDiag");
}

void BaseGameSettingsDialog::m_OnButtonPressed(Input::Button button)
{
    if (!m_active || m_closing)
        return;

    switch (button)
    {
    case Input::Button::FX_0:
        if (g_input.GetButton(Input::Button::FX_1))
            Close();
        break;
    case Input::Button::FX_1:
        if (g_input.GetButton(Input::Button::FX_0))
            Close();
        break;
    case Input::Button::Back:
        Close();
        break;

    default:
        break;
    }
}

void BaseGameSettingsDialog::m_OnButtonReleased(Input::Button button)
{
    if (!m_active || m_closing || !m_enableFXInputs)
        return;

    switch (button)
    {
    case Input::Button::FX_0:
        m_AdvanceTab(-1);
        break;
    case Input::Button::FX_1:
        m_AdvanceTab(1);
        break;
    case Input::Button::BT_0:
        m_ChangeStepSetting(-5);
        break;
    case Input::Button::BT_1:
        m_ChangeStepSetting(-1);
        break;
    case Input::Button::BT_2:
        m_ChangeStepSetting(1);
        break;
    case Input::Button::BT_3:
        m_ChangeStepSetting(5);
        break;
    default:
        break;
    }
}

inline static void AdvanceLooping(int& value, int steps, int size)
{
    value = (steps + value) % size;
    if (value < 0)
    {
        value = size + value;
    }
}

void BaseGameSettingsDialog::m_AdvanceSelection(int steps)
{
    if (m_tabs[m_currentTab]->settings.size() == 0)
        m_currentSetting = 0;
    else
        AdvanceLooping(m_currentSetting, steps, m_tabs[m_currentTab]->settings.size());

    if (steps != 0)
        m_knobAdvance[1] = 0.0f;
}
void BaseGameSettingsDialog::m_AdvanceTab(int steps)
{
    if (steps == 0)
        return;

    m_currentSetting = 0;
    AdvanceLooping(m_currentTab, steps, m_tabs.size());
    
    OnAdvanceTab();
}

void BaseGameSettingsDialog::m_ChangeStepSetting(int steps)
{
    if (steps == 0)
        return;

    if (m_currentSetting >= m_tabs[m_currentTab]->settings.size())
        return;

    auto currentSetting = m_tabs[m_currentTab]->settings.at(m_currentSetting).get();

    switch (currentSetting->type)
    {
    case SettingType::Integer:
        currentSetting->intSetting.val = Math::Clamp(currentSetting->intSetting.val + steps * currentSetting->intSetting.step,
            currentSetting->intSetting.min,
            currentSetting->intSetting.max);
        currentSetting->intSetting.setter.Call(currentSetting->intSetting.val);
        break;
    case SettingType::Toggle:
    case SettingType::Button:
        currentSetting->boolSetting.val = !currentSetting->boolSetting.val;
        currentSetting->boolSetting.setter.Call(currentSetting->boolSetting.val);
        break;
    case SettingType::Enum:
        int size = currentSetting->enumSetting.options.size();
        AdvanceLooping(currentSetting->enumSetting.val, steps, size);
        String& newVal = currentSetting->enumSetting.options.at(currentSetting->enumSetting.val);
        currentSetting->enumSetting.setter.Call(newVal);
        break;
    }
}

void BaseGameSettingsDialog::m_OnKeyPressed(SDL_Scancode code)
{
    if (!m_active || m_closing)
        return;

    switch (code)
    {
    case SDL_SCANCODE_LEFT:
        m_ChangeStepSetting(-1);
        break;
    case SDL_SCANCODE_RIGHT:
        m_ChangeStepSetting(1);
        break;
    case SDL_SCANCODE_TAB:
        m_AdvanceTab(1);
        break;
    case SDL_SCANCODE_UP:
        m_AdvanceSelection(-1);
        break;
    case SDL_SCANCODE_DOWN:
        m_AdvanceSelection(1);
        break;
    }
}