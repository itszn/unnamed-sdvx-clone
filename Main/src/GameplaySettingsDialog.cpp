#include "stdafx.h"
#include "GameplaySettingsDialog.hpp"
#include "lua.hpp"
#include "Application.hpp"

GameplaySettingsDialog::~GameplaySettingsDialog()
{
    if (m_lua)
    {
        g_application->DisposeLua(m_lua);
        m_lua = nullptr;
    }

    for (auto &tab : m_tabs)
    {
        tab->settings.clear();
    }
    m_tabs.clear();

    g_input.OnButtonReleased.RemoveAll(this);
    g_input.OnButtonPressed.RemoveAll(this);
    g_gameWindow->OnKeyPressed.RemoveAll(this);
}

template <typename EnumClass>
GameplaySettingsDialog::Setting GameplaySettingsDialog::m_CreateEnumSetting(GameConfigKeys key, String name)
{
    Setting s = std::make_unique<SettingData>(SettingData({
        name,
        SettingType::Enum,
    }));

    EnumStringMap<typename EnumClass::EnumType> nameMap = EnumClass::GetMap();
    for (auto it = nameMap.begin(); it != nameMap.end(); it++)
    {
        s->enumSetting.options.Add(*(*it).second);
    }
    String current = EnumClass::ToString(g_gameConfig.GetEnum<EnumClass>(key));

    s->enumSetting.val = std::distance(s->enumSetting.options.begin(), std::find(s->enumSetting.options.begin(), s->enumSetting.options.end(), current));

    auto getter = [key](String &value) {
        value = EnumClass::ToString(g_gameConfig.GetEnum<EnumClass>(key));
    };

    auto setter = [key](String newValue) {
        g_gameConfig.SetEnum<EnumClass>(key, EnumClass::FromString(newValue));
    };
    s->enumSetting.getter.AddLambda(std::move(getter));
    s->enumSetting.setter.AddLambda(std::move(setter));

    return s;
}

GameplaySettingsDialog::Setting GameplaySettingsDialog::m_CreateFloatSetting(GameConfigKeys key, String name, Vector2 range, float mult)
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

GameplaySettingsDialog::Setting GameplaySettingsDialog::m_CreateIntSetting(GameConfigKeys key, String name, Vector2i range)
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
    s->intSetting.setter.AddLambda(std::move(setter));
    s->intSetting.getter.AddLambda(std::move(getter));
    return s;
}

GameplaySettingsDialog::Setting GameplaySettingsDialog::m_CreateToggleSetting(GameConfigKeys key, String name)
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

bool GameplaySettingsDialog::Init()
{
    Tab offsetTab = std::make_unique<TabData>();
    offsetTab->name = "Offsets";
    offsetTab->settings.push_back(m_CreateIntSetting(GameConfigKeys::GlobalOffset, "Global Offset", {-200, 200}));
    offsetTab->settings.push_back(m_CreateIntSetting(GameConfigKeys::InputOffset, "Input Offset", {-200, 200}));

    Tab speedTab = std::make_unique<TabData>();
    speedTab->name = "HiSpeed";
    speedTab->settings.push_back(m_CreateEnumSetting<Enum_SpeedMods>(GameConfigKeys::SpeedMod, "Speed Mod"));
    speedTab->settings.push_back(m_CreateFloatSetting(GameConfigKeys::HiSpeed, "HiSpeed", {0.1f, 16.f}));
    speedTab->settings.push_back(m_CreateFloatSetting(GameConfigKeys::ModSpeed, "ModSpeed", {50, 1500}, 20.0f));

    Tab gameTab = std::make_unique<TabData>();
    gameTab->name = "Game";
    gameTab->settings.push_back(m_CreateEnumSetting<Enum_GaugeTypes>(GameConfigKeys::GaugeType, "Gauge"));
    gameTab->settings.push_back(m_CreateToggleSetting(GameConfigKeys::RandomizeChart, "Random"));
    gameTab->settings.push_back(m_CreateToggleSetting(GameConfigKeys::MirrorChart, "Mirror"));
    gameTab->settings.push_back(m_CreateToggleSetting(GameConfigKeys::DisableBackgrounds, "Hide Backgrounds"));

    Tab hidsudTab = std::make_unique<TabData>();
    hidsudTab->name = "Hid/Sud";
    hidsudTab->settings.push_back(m_CreateFloatSetting(GameConfigKeys::HiddenCutoff, "Hidden Cutoff", { 0.f, 1.f }));
    hidsudTab->settings.push_back(m_CreateFloatSetting(GameConfigKeys::HiddenFade, "Hidden Fade", { 0.f, 1.f }));
    hidsudTab->settings.push_back(m_CreateFloatSetting(GameConfigKeys::SuddenCutoff, "Sudden Cutoff", { 0.f, 1.f }));
    hidsudTab->settings.push_back(m_CreateFloatSetting(GameConfigKeys::SuddenFade, "Sudden Fade", { 0.f, 1.f }));
    hidsudTab->settings.push_back(m_CreateToggleSetting(GameConfigKeys::ShowCover, "Show Track Cover"));


    m_tabs.push_back(std::move(offsetTab));
    m_tabs.push_back(std::move(speedTab));
    m_tabs.push_back(std::move(gameTab));
    m_tabs.push_back(std::move(hidsudTab));

    m_lua = g_application->LoadScript("gamesettingsdialog");
    if (!m_lua)
    {
        return false;
    }

    m_SetTables();
    g_input.OnButtonPressed.Add(this, &GameplaySettingsDialog::m_OnButtonPressed);
    g_input.OnButtonReleased.Add(this, &GameplaySettingsDialog::m_OnButtonReleased);
    g_gameWindow->OnKeyPressed.Add(this, &GameplaySettingsDialog::m_OnKeyPressed);

    m_isInitialized = true;

    return true;
}
void GameplaySettingsDialog::Tick(float deltaTime)
{
    m_active = m_targetActive;
    if (!m_active)
        return;

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

    auto currentSetting = m_tabs[m_currentTab]->settings.at(m_currentSetting).get();
    if (currentSetting->type == Floating)
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

void GameplaySettingsDialog::Render(float deltaTime)
{
    m_SetTables();
    lua_getglobal(m_lua, "render");
    lua_pushnumber(m_lua, deltaTime);
    lua_pushboolean(m_lua, m_active);
    if (lua_pcall(m_lua, 2, 0, 0) != 0)
    {
        Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
        g_gameWindow->ShowMessageBox("Lua Error", lua_tostring(m_lua, -1), 0);
        assert(false);
    }
    lua_settop(m_lua, 0);
}

void GameplaySettingsDialog::m_SetTables()
{
    assert(m_lua);

    auto pushStringToTable = [&](const char *name, String data) {
        lua_pushstring(m_lua, name);
        lua_pushstring(m_lua, data.c_str());
        lua_settable(m_lua, -3);
    };
    auto pushIntToTable = [&](const char *name, int data) {
        lua_pushstring(m_lua, name);
        lua_pushinteger(m_lua, data);
        lua_settable(m_lua, -3);
    };
    auto pushBoolToTable = [&](const char *name, bool data) {
        lua_pushstring(m_lua, name);
        lua_pushboolean(m_lua, data);
        lua_settable(m_lua, -3);
    };
    auto pushFloatToTable = [&](const char *name, float data) {
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
            for (auto &&tab : m_tabs)
            {
                lua_pushinteger(m_lua, ++tabCounter);
                lua_newtable(m_lua);
                pushStringToTable("name", tab->name);
                lua_pushstring(m_lua, "settings");
                lua_newtable(m_lua);
                {
                    int settingCounter = 0;
                    for (auto &&setting : tab->settings)
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
                            setting->boolSetting.getter.Call(setting->boolSetting.val);
                            pushStringToTable("type", "toggle");
                            pushBoolToTable("value", setting->boolSetting.val);
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
                            for (auto &&o : setting->enumSetting.options)
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

void GameplaySettingsDialog::Open()
{
    assert(!m_active);
    m_targetActive = true;
}

void GameplaySettingsDialog::Close()
{
    assert(m_active);
    m_targetActive = false;
}

bool GameplaySettingsDialog::IsActive()
{
    return m_active;
}

bool GameplaySettingsDialog::IsInitialized()
{
    return m_isInitialized;
}

void GameplaySettingsDialog::m_OnButtonPressed(Input::Button button)
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

void GameplaySettingsDialog::m_OnButtonReleased(Input::Button button)
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

inline void AdvanceLooping(int &value, int steps, int size)
{
    value = (steps + value) % size;
    if (value < 0)
    {
        value = size + value;
    }
}

void GameplaySettingsDialog::m_AdvanceSelection(int steps)
{
    AdvanceLooping(m_currentSetting, steps, m_tabs[m_currentTab]->settings.size());
    if (steps != 0)
        m_knobAdvance[1] = 0.0f;
}
void GameplaySettingsDialog::m_AdvanceTab(int steps)
{
    if (steps == 0)
        return;

    m_currentSetting = 0;
    AdvanceLooping(m_currentTab, steps, m_tabs.size());
}

void GameplaySettingsDialog::m_ChangeStepSetting(int steps)
{
    if (steps == 0)
        return;

    auto currentSetting = m_tabs[m_currentTab]->settings.at(m_currentSetting).get();

    switch (currentSetting->type)
    {
    case SettingType::Integer:
        currentSetting->intSetting.val = Math::Clamp(currentSetting->intSetting.val + steps,
                                                     currentSetting->intSetting.min,
                                                     currentSetting->intSetting.max);
        currentSetting->intSetting.setter.Call(currentSetting->intSetting.val);
        break;
    case SettingType::Toggle:
        currentSetting->boolSetting.val = !currentSetting->boolSetting.val;
        currentSetting->boolSetting.setter.Call(currentSetting->boolSetting.val);
        break;
    case SettingType::Enum:
        int size = currentSetting->enumSetting.options.size();
        AdvanceLooping(currentSetting->enumSetting.val, steps, size);
        String &newVal = currentSetting->enumSetting.options.at(currentSetting->enumSetting.val);
        currentSetting->enumSetting.setter.Call(newVal);
        break;
    }
}

void GameplaySettingsDialog::m_OnKeyPressed(int32 key)
{
    if (!m_active || m_closing)
        return;

    switch (key)
    {
    case SDLK_LEFT:
        m_ChangeStepSetting(-1);
        break;
    case SDLK_RIGHT:
        m_ChangeStepSetting(1);
        break;
    case SDLK_TAB:
        m_AdvanceTab(1);
        break;
    case SDLK_UP:
        m_AdvanceSelection(-1);
        break;
    case SDLK_DOWN:
        m_AdvanceSelection(1);
        break;
    }
}