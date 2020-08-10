#pragma once
#include "BaseGameSettingsDialog.hpp"

class GameplaySettingsDialog: public BaseGameSettingsDialog
{
public:
    GameplaySettingsDialog();

    void InitTabs() override;
    void OnAdvanceTab() override;

    Delegate<> onPressAutoplay;
    Delegate<> onPressPractice;
};