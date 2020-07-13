#include "stdafx.h"
#include "GameplaySettingsDialog.hpp"

void GameplaySettingsDialog::InitTabs()
{
    Tab offsetTab = std::make_unique<TabData>();
    offsetTab->name = "Offsets";
    offsetTab->settings.push_back(CreateIntSetting(GameConfigKeys::GlobalOffset, "Global Offset", {-200, 200}));
    offsetTab->settings.push_back(CreateIntSetting(GameConfigKeys::InputOffset, "Input Offset", {-200, 200}));

    Tab speedTab = std::make_unique<TabData>();
    speedTab->name = "HiSpeed";
    speedTab->settings.push_back(CreateEnumSetting<Enum_SpeedMods>(GameConfigKeys::SpeedMod, "Speed Mod"));
    speedTab->settings.push_back(CreateFloatSetting(GameConfigKeys::HiSpeed, "HiSpeed", {0.1f, 16.f}));
    speedTab->settings.push_back(CreateFloatSetting(GameConfigKeys::ModSpeed, "ModSpeed", {50, 1500}, 20.0f));

    Tab gameTab = std::make_unique<TabData>();
    gameTab->name = "Game";
    gameTab->settings.push_back(CreateEnumSetting<Enum_GaugeTypes>(GameConfigKeys::GaugeType, "Gauge"));
    gameTab->settings.push_back(CreateToggleSetting(GameConfigKeys::RandomizeChart, "Random"));
    gameTab->settings.push_back(CreateToggleSetting(GameConfigKeys::MirrorChart, "Mirror"));
    gameTab->settings.push_back(CreateToggleSetting(GameConfigKeys::DisableBackgrounds, "Hide Backgrounds"));
    gameTab->settings.push_back(CreateEnumSetting<Enum_ScoreDisplayModes>(GameConfigKeys::ScoreDisplayMode, "Score Display"));

    Tab hidsudTab = std::make_unique<TabData>();
    hidsudTab->name = "Hid/Sud";
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::HiddenCutoff, "Hidden Cutoff", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::HiddenFade, "Hidden Fade", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::SuddenCutoff, "Sudden Cutoff", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::SuddenFade, "Sudden Fade", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateToggleSetting(GameConfigKeys::ShowCover, "Show Track Cover"));

    AddTab(std::move(offsetTab));
    AddTab(std::move(speedTab));
    AddTab(std::move(gameTab));
    AddTab(std::move(hidsudTab));

    SetCurrentTab(g_gameConfig.GetInt(GameConfigKeys::GameplaySettingsDialogLastTab));
}

void GameplaySettingsDialog::OnAdvanceTab()
{
    g_gameConfig.Set(GameConfigKeys::GameplaySettingsDialogLastTab, GetCurrentTab());
}
