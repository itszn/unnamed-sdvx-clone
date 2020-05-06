#pragma once
#include "Application.hpp"
#include "Input.hpp"
#include "ApplicationTickable.hpp"
#include "GameConfig.hpp"

class GameplaySettingsDialog
{
public:
    ~GameplaySettingsDialog();
    bool Init();
    void Tick(float deltaTime);
    void Render(float deltaTime);
    void Open();

    //Call to start closing the dialog
    void Close();
    bool IsActive();
    bool IsInitialized();

private:
    enum SettingType
    {
        Integer,
        Floating,
        Toggle,
        Enum
    };

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
            Delegate<String &> getter;
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

    void m_ChangeState();
    void m_SetTables();
    void m_AdvanceSelection(int steps);
    void m_AdvanceTab(int steps);
    void m_ChangeStepSetting(int steps); //int, enum, toggle, are all advanced in distinct steps
    void m_OnButtonPressed(Input::Button button);
    void m_OnButtonReleased(Input::Button button);
    void m_OnKeyPressed(int32 key);
    Setting m_CreateToggleSetting(GameConfigKeys key, String name);
    Setting m_CreateIntSetting(GameConfigKeys key, String name, Vector2i range);
    Setting m_CreateFloatSetting(GameConfigKeys key, String name, Vector2 range, float mult = 1.0f);

    template <typename EnumClass>
    Setting m_CreateEnumSetting(GameConfigKeys key, String name);

    //Call when closing has been completed
    void m_Finish();

    struct lua_State *m_lua = nullptr;
    int m_currentId;
    float m_knobAdvance[2] = {0.0f, 0.0f};
    float m_sensMult = 1.0f;
    bool m_active = false;
    //set a target in open/close and apply it in the next tick because stuff
    bool m_targetActive = false;
    bool m_closing = false;
    bool m_isInitialized = false;
    bool m_enableFXInputs = false;
    int m_currentTab = 0;
    int m_currentSetting = 0;
    Vector<Tab> m_tabs;
};