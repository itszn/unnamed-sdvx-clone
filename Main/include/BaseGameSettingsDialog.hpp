#pragma once

#include "GameConfig.hpp"
#include "Input.hpp"

enum class SettingType
{
    Integer,
    Floating,
    Toggle,
    Enum
};

// Base class for popup dialog for game settings
class BaseGameSettingsDialog
{
public:
    typedef struct SettingData
    {
        String name;
        SettingType type;
        struct
        {
            float val;
            float min;
            float max;
            float mult; //multiply input by
            Delegate<float&> getter;
            Delegate<float> setter;
        } floatSetting;

        struct
        {
            int val;
            int min;
            int max;
            Delegate<int&> getter;
            Delegate<int> setter;
        } intSetting;

        struct
        {
            int val;
            Vector<String> options;
            Delegate<String&> getter;
            Delegate<String> setter;
        } enumSetting;

        struct
        {
            bool val;
            Delegate<bool&> getter;
            Delegate<bool> setter;
        } boolSetting;

    } SettingData;
    typedef std::unique_ptr<SettingData> Setting;

    typedef struct TabData
    {
        String name;
        Vector<Setting> settings;
    } TabData;
    typedef std::unique_ptr<TabData> Tab;

public:
    ~BaseGameSettingsDialog();

    void Tick(float deltaTime);
    void Render(float deltaTime);
    bool Init();

    inline void AddTab(Tab tab) { m_tabs.emplace_back(std::move(tab)); }

    inline bool IsActive() const { return m_active; }
    inline bool IsInitialized() const { return m_isInitialized; }

    inline void Open() { assert(!m_active); m_targetActive = true; }
    inline void Close() { assert(m_active); m_targetActive = false; }

protected:
    virtual void InitTabs() = 0;
    virtual void OnAdvanceTab() {};

    Setting CreateToggleSetting(GameConfigKeys key, String name);
    Setting CreateIntSetting(GameConfigKeys key, String name, Vector2i range);
    Setting CreateFloatSetting(GameConfigKeys key, String name, Vector2 range, float mult = 1.0f);

    template <typename EnumClass>
    Setting CreateEnumSetting(GameConfigKeys key, String name);

    inline int GetCurrentTab() const { return m_currentTab; }
    inline void SetCurrentTab(int tabIndex) { m_currentTab = tabIndex; }

private:
    void m_SetTables();
    void m_AdvanceSelection(int steps);
    void m_AdvanceTab(int steps);
    void m_ChangeStepSetting(int steps); //int, enum, toggle, are all advanced in distinct steps
    void m_OnButtonPressed(Input::Button button);
    void m_OnButtonReleased(Input::Button button);
    void m_OnKeyPressed(SDL_Scancode code);

    // Set a target in open/close and apply it in the next tick because stuff
    bool m_targetActive = false;
    bool m_active = false;
    bool m_closing = false;
    bool m_isInitialized = false;

    bool m_enableFXInputs = false;

    int m_currentTab = 0;
    int m_currentSetting = 0;

    float m_knobAdvance[2] = {0.0f, 0.0f};
    float m_sensMult = 1.0f;

    struct lua_State* m_lua = nullptr;
    Vector<Tab> m_tabs;
};

template <typename EnumClass>
BaseGameSettingsDialog::Setting BaseGameSettingsDialog::CreateEnumSetting(GameConfigKeys key, String name)
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

    auto getter = [key](String& value) {
        value = EnumClass::ToString(g_gameConfig.GetEnum<EnumClass>(key));
    };

    auto setter = [key](String newValue) {
        g_gameConfig.SetEnum<EnumClass>(key, EnumClass::FromString(newValue));
    };
    s->enumSetting.getter.AddLambda(std::move(getter));
    s->enumSetting.setter.AddLambda(std::move(setter));

    return s;
}