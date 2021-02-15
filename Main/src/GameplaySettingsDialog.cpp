#include "stdafx.h"
#include "GameplaySettingsDialog.hpp"
#include "HitStat.hpp"
#include "Application.hpp"
#include "SongSelect.hpp"
#include "shared/Files.hpp"
#include "GuiUtils.hpp"

GameplaySettingsDialog::GameplaySettingsDialog(SongSelect* songSelectScreen)
    : songSelectScreen(songSelectScreen)
{
}



void GameplaySettingsDialog::InitTabs()
{
    Tab offsetTab = std::make_unique<TabData>();
    offsetTab->name = "Offsets";
    offsetTab->settings.push_back(CreateIntSetting(GameConfigKeys::GlobalOffset, "Global Offset", {-200, 200}));
    offsetTab->settings.push_back(CreateIntSetting(GameConfigKeys::InputOffset, "Input Offset", {-200, 200}));
    if (songSelectScreen != nullptr)
		offsetTab->settings.push_back(m_CreateSongOffsetSetting());

    Tab speedTab = std::make_unique<TabData>();
    speedTab->name = "HiSpeed";
    speedTab->settings.push_back(CreateEnumSetting<Enum_SpeedMods>(GameConfigKeys::SpeedMod, "Speed Mod"));
    speedTab->settings.push_back(CreateFloatSetting(GameConfigKeys::HiSpeed, "HiSpeed", {0.1f, 16.f}));
    speedTab->settings.push_back(CreateFloatSetting(GameConfigKeys::ModSpeed, "ModSpeed", {50, 1500}, 20.0f));

    Tab gameTab = std::make_unique<TabData>();
    gameTab->name = "Game";
    gameTab->settings.push_back(CreateEnumSetting<Enum_GaugeTypes>(GameConfigKeys::GaugeType, "Gauge"));
    gameTab->settings.push_back(CreateBoolSetting(GameConfigKeys::RandomizeChart, "Random"));
    gameTab->settings.push_back(CreateBoolSetting(GameConfigKeys::MirrorChart, "Mirror"));
    gameTab->settings.push_back(CreateBoolSetting(GameConfigKeys::DisableBackgrounds, "Hide Backgrounds"));
    gameTab->settings.push_back(CreateEnumSetting<Enum_ScoreDisplayModes>(GameConfigKeys::ScoreDisplayMode, "Score Display"));
    if (songSelectScreen != nullptr)
    {
        gameTab->settings.push_back(CreateButton("Autoplay", [this](const auto&) { onPressAutoplay.Call(); }));
        gameTab->settings.push_back(CreateButton("Practice", [this](const auto&) { onPressPractice.Call(); }));
    }

    Tab hidsudTab = std::make_unique<TabData>();
    hidsudTab->name = "Hid/Sud";
    hidsudTab->settings.push_back(CreateBoolSetting(GameConfigKeys::EnableHiddenSudden, "Enable Hidden / Sudden"));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::HiddenCutoff, "Hidden Cutoff", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::HiddenFade, "Hidden Fade", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::SuddenCutoff, "Sudden Cutoff", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateFloatSetting(GameConfigKeys::SuddenFade, "Sudden Fade", { 0.f, 1.f }));
    hidsudTab->settings.push_back(CreateBoolSetting(GameConfigKeys::ShowCover, "Show Track Cover"));

    Tab judgeWindowTab = std::make_unique<TabData>();
    judgeWindowTab->name = "Judgement";
    judgeWindowTab->settings.push_back(CreateIntSetting(GameConfigKeys::HitWindowPerfect, "Crit Window", {0, HitWindow::NORMAL.perfect}));
    judgeWindowTab->settings.push_back(CreateIntSetting(GameConfigKeys::HitWindowGood, "Near Window", { 0, HitWindow::NORMAL.good }));
    judgeWindowTab->settings.push_back(CreateIntSetting(GameConfigKeys::HitWindowHold, "Hold Window", { 0, HitWindow::NORMAL.hold }));
    judgeWindowTab->settings.push_back(CreateButton("Set to NORMAL", [](const auto&) { HitWindow::NORMAL.SaveConfig(); }));
    judgeWindowTab->settings.push_back(CreateButton("Set to HARD", [](const auto&) { HitWindow::HARD.SaveConfig(); }));


    Tab profileWindowTab = std::make_unique<TabData>();
    profileWindowTab->name = "Profiles";


	profileWindowTab->settings.push_back(m_CreateProfileSetting("Main"));
	{

        Vector<FileInfo> files = Files::ScanFiles(
            Path::Absolute("profiles/"), "cfg", NULL);

        for (auto &file : files)
        {

            String profileName = "";
            String unused = Path::RemoveLast(file.fullPath, &profileName);
            profileName = profileName.substr(0, profileName.length() - 4); // Remove .cfg

			profileWindowTab->settings.push_back(m_CreateProfileSetting(profileName));
        }
    }
	auto this_p = this;
    profileWindowTab->settings.push_back(CreateButton("Create Profile", [this_p](const auto&) {
		BasicPrompt *w = new BasicPrompt("Create New Profile","Enter name for profile:\n(Enter existing to overwrite)","Create");
        w->OnResult.AddLambda([this_p](bool valid, const char* data) {
            if (!valid || strlen(data) == 0)
                return;

            String profile = String(data);
            // Validate filename (this is windows specific but is a subperset of linux)
            // https://stackoverflow.com/questions/4814040/allowed-characters-in-filename/35352640#35352640
			// TODO we could probably make a general function under Path::
			profile.erase(std::remove_if(profile.begin(), profile.end(),
				[](unsigned char x) {
					switch (x) {
					case '\0':
					case '\\':
					case '/':
					case ':':
					case '*':
					case '"':
					case '<':
					case '>':
					case '|':
					case '\n':
					case '\r':
						return true;
					default:
						return false;
					}
				}
			), profile.end());

			if (profile == "." || profile == "..")
				return;
            if (profile[0] == ' '
					|| profile[profile.length() - 1] == ' '
					|| profile[profile.length() - 1] == '.')
                return;

			if (!Path::IsDirectory(Path::Absolute("profiles")))
				Path::CreateDir(Path::Absolute("profiles"));

			File profileSelected;
            if (profileSelected.OpenWrite(Path::Absolute(Path::Normalize("profiles/selected.txt"))))
            {
                profileSelected.Write(*profile, profile.length());
                profileSelected.Close();

                // Save old setting
                g_application->ApplySettings();
                // Update with new profile name to clone
                g_gameConfig.Set(GameConfigKeys::CurrentProfileName, profile);
                // Now save as new profile
                g_application->ApplySettings();
                // Re-init tabs
                this_p->ResetTabs();
            }
		});
		w->Focus();
		g_application->AddTickable(w);
    }));

    AddTab(std::move(offsetTab));
    AddTab(std::move(speedTab));
    AddTab(std::move(gameTab));
    AddTab(std::move(hidsudTab));
    AddTab(std::move(judgeWindowTab));
    AddTab(std::move(profileWindowTab));

    SetCurrentTab(g_gameConfig.GetInt(GameConfigKeys::GameplaySettingsDialogLastTab));
}

void GameplaySettingsDialog::OnAdvanceTab()
{
    g_gameConfig.Set(GameConfigKeys::GameplaySettingsDialogLastTab, GetCurrentTab());
}

GameplaySettingsDialog::Setting GameplaySettingsDialog::m_CreateSongOffsetSetting()
{
    Setting songOffsetSetting = std::make_unique<SettingData>("Song Offset", SettingType::Integer);
    songOffsetSetting->name = "Song Offset";
    songOffsetSetting->type = SettingType::Integer;
    songOffsetSetting->intSetting.val = 0;
    songOffsetSetting->intSetting.min = -200;
    songOffsetSetting->intSetting.max = 200;
    songOffsetSetting->setter.AddLambda([this](const auto& data) { onSongOffsetChange.Call(data.intSetting.val); });
    songOffsetSetting->getter.AddLambda([this](auto& data) {
        if (const ChartIndex* chart = songSelectScreen->GetCurrentSelectedChart())
        {
            data.intSetting.val = chart->custom_offset;
        }
    });

    return songOffsetSetting;
}

GameplaySettingsDialog::Setting GameplaySettingsDialog::m_CreateProfileSetting(const String& profileName) {
	Setting s = std::make_unique<SettingData>(profileName, SettingType::Boolean);
	auto getter = [profileName](SettingData& data)
	{
		data.boolSetting.val = (g_gameConfig.GetString(GameConfigKeys::CurrentProfileName) == profileName);
	};

	Application* app = g_application;

	auto setter = [app, profileName](const SettingData& data)
	{

		if (data.boolSetting.val) {
			if (!Path::IsDirectory(Path::Absolute("profiles")))
				Path::CreateDir(Path::Absolute("profiles"));

			File profileSelected;
			if (profileSelected.OpenWrite(Path::Absolute(Path::Normalize("profiles/selected.txt"))))
			{
				profileSelected.Write(*profileName, profileName.length());
				profileSelected.Close();

				// I don't know why I can't just use g_application in this lambda

				// Save old setting
				app->ApplySettings();

				// Load new profile
				app->ReloadConfig();
			}
		}
	};

	s->boolSetting.val = (g_gameConfig.GetString(GameConfigKeys::CurrentProfileName) == profileName);
	s->setter.AddLambda(std::move(setter));
	s->getter.AddLambda(std::move(getter));
	return s;
}
