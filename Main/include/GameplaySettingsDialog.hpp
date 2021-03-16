#pragma once
#include "BaseGameSettingsDialog.hpp"

class SongSelect;

class GameplaySettingsDialog: public BaseGameSettingsDialog
{
public:
    GameplaySettingsDialog(SongSelect* songSelectScreen);
    GameplaySettingsDialog() {};

    void InitTabs() override;
    void OnAdvanceTab() override;

    Delegate<> onPressAutoplay;
    Delegate<> onPressPractice;
    Delegate<int> onSongOffsetChange;
    Delegate<> onPressComputeSongOffset;

private:
    SongSelect* songSelectScreen = nullptr;
    Setting m_CreateSongOffsetSetting();
};