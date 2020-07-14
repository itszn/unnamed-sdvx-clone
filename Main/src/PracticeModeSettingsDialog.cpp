#include "stdafx.h"
#include "PracticeModeSettingsDialog.hpp"

PracticeModeSettingsDialog::PracticeModeSettingsDialog(MapTime endTime, MapTimeRange& range)
    : m_endTime(endTime), m_range(range)
{
}

void PracticeModeSettingsDialog::InitTabs()
{
    AddTab(std::move(m_CreatePlaybackTab()));
    AddTab(std::move(m_CreateConditionTab()));

    SetCurrentTab(0);
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreatePlaybackTab()
{
    Tab playbackTab = std::make_unique<TabData>();
    playbackTab->name = "Playback";

    Setting useBTForLoopSetting = std::make_unique<SettingData>();
    useBTForLoopSetting->name = "Enable setting points by pressing BT A or B";
    useBTForLoopSetting->type = SettingType::Toggle;
    useBTForLoopSetting->boolSetting.val = false;

    playbackTab->settings.emplace_back(std::move(useBTForLoopSetting));

    Setting loopBeginSetting = std::make_unique<SettingData>();
    loopBeginSetting->name = "Start point";
    loopBeginSetting->type = SettingType::Integer;
    loopBeginSetting->intSetting.val = 0;
    loopBeginSetting->intSetting.min = 0;
    loopBeginSetting->intSetting.max = m_endTime;

    m_loopBegin = loopBeginSetting.get();
    playbackTab->settings.emplace_back(std::move(loopBeginSetting));

    Setting loopEndSetting = std::make_unique<SettingData>();
    loopEndSetting->name = "End point";
    loopEndSetting->type = SettingType::Integer;
    loopEndSetting->intSetting.val = 0;
    loopEndSetting->intSetting.min = 0;
    loopEndSetting->intSetting.max = m_endTime;

    m_loopEnd = loopEndSetting.get();
    playbackTab->settings.emplace_back(std::move(loopEndSetting));

    Setting loopOnSuccess = std::make_unique<SettingData>();
    loopOnSuccess->name = "Loop on success";
    loopOnSuccess->type = SettingType::Toggle;
    loopOnSuccess->boolSetting.val = true;

    playbackTab->settings.emplace_back(std::move(loopOnSuccess));

    Setting loopOnFail = std::make_unique<SettingData>();
    loopOnFail->name = "Loop on fail";
    loopOnFail->type = SettingType::Toggle;
    loopOnFail->boolSetting.val = true;

    playbackTab->settings.emplace_back(std::move(loopOnFail));

    Setting useBTForSpeedSetting = std::make_unique<SettingData>();
    useBTForSpeedSetting->name = "Enable setting speed by pressing BT C or D";
    useBTForSpeedSetting->type = SettingType::Toggle;
    useBTForSpeedSetting->boolSetting.val = false;

    playbackTab->settings.emplace_back(std::move(useBTForSpeedSetting));

    Setting speedSetting = std::make_unique<SettingData>();
    speedSetting->name = "Playback speed";
    speedSetting->type = SettingType::Floating;
    speedSetting->floatSetting.val = 1.0f;
    speedSetting->floatSetting.min = 0.25f;
    speedSetting->floatSetting.max = 2.0f;
    speedSetting->floatSetting.mult = 0.25f;

    m_speed = speedSetting.get();
    playbackTab->settings.emplace_back(std::move(speedSetting));

    return playbackTab;
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateConditionTab()
{
    Tab conditionTab = std::make_unique<TabData>();
    conditionTab->name = "Condition";

    return conditionTab;
}
