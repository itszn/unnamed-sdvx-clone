#include "stdafx.h"
#include "PracticeModeSettingsDialog.hpp"

void PracticeModeSettingsDialog::InitTabs()
{
    Tab instructionTab = std::make_unique<TabData>();
    instructionTab->name = "Instruction";

    Tab playbackTab = std::make_unique<TabData>();
    playbackTab->name = "Playback";

    Tab conditionTab = std::make_unique<TabData>();
    conditionTab->name = "Condition";


    AddTab(std::move(instructionTab));
    AddTab(std::move(playbackTab));
    AddTab(std::move(conditionTab));

    SetCurrentTab(0);
}