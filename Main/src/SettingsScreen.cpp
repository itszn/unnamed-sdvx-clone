#include "stdafx.h"
#include "SettingsScreen.hpp"

#include <Shared/Profiling.hpp>
#include <Shared/Enum.hpp>
#include <Shared/Files.hpp>

#include <Audio/Audio.hpp>

#include "Application.hpp"
#include "GameConfig.hpp"
#include "SkinConfig.hpp"
#include "Scoring.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "Shared/Jobs.hpp"
#include "ScoreScreen.hpp"
#include "Input.hpp"
#include "nanovg.h"
#include "CalibrationScreen.hpp"
#include "TransitionScreen.hpp"
#include "GuiUtils.hpp"
#include "SettingsPage.hpp"

static inline const char* GetKeyNameFromScancodeConfig(int scancode)
{
	return SDL_GetKeyName(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode)));
}

class SettingsPage_Input : public SettingsPage
{
public:
	SettingsPage_Input(nk_context* nctx) : SettingsPage(nctx, "Input") {}

protected:
	void Load() override
	{
		m_gamePads.clear();

		m_gamePadsStr = g_gameWindow->GetGamepadDeviceNames();
		for (const String& gamePadName : m_gamePadsStr)
		{
			m_gamePads.Add(gamePadName.data());
		}
	}
	
	void Save() override
	{
		if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Mouse)
		{
			g_gameConfig.SetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice, InputDevice::Keyboard);
		}
	}

	Vector<String> m_gamePadsStr;
	Vector<const char*> m_gamePads;

	String m_controllerButtonNames[8];
	String m_controllerLaserNames[2];

	const Vector<GameConfigKeys>* m_activeBTKeys = &m_keyboardKeys;
	const Vector<GameConfigKeys>* m_activeLaserKeys = &m_keyboardLaserKeys;
	bool m_useBTGamepad = false;
	bool m_useLaserGamepad = false;
	bool m_altBinds = false;

	const Vector<GameConfigKeys> m_keyboardKeys = {
		GameConfigKeys::Key_BTS,
		GameConfigKeys::Key_BT0,
		GameConfigKeys::Key_BT1,
		GameConfigKeys::Key_BT2,
		GameConfigKeys::Key_BT3,
		GameConfigKeys::Key_FX0,
		GameConfigKeys::Key_FX1,
		GameConfigKeys::Key_Back
	};
	const Vector<GameConfigKeys> m_altKeyboardKeys = {
		GameConfigKeys::Key_BTSAlt,
		GameConfigKeys::Key_BT0Alt,
		GameConfigKeys::Key_BT1Alt,
		GameConfigKeys::Key_BT2Alt,
		GameConfigKeys::Key_BT3Alt,
		GameConfigKeys::Key_FX0Alt,
		GameConfigKeys::Key_FX1Alt,
		GameConfigKeys::Key_BackAlt
	};

	const Vector<GameConfigKeys> m_keyboardLaserKeys = {
		GameConfigKeys::Key_Laser0Neg,
		GameConfigKeys::Key_Laser0Pos,
		GameConfigKeys::Key_Laser1Neg,
		GameConfigKeys::Key_Laser1Pos,
	};
	const Vector<GameConfigKeys> m_altKeyboardLaserKeys = {
		GameConfigKeys::Key_Laser0NegAlt,
		GameConfigKeys::Key_Laser0PosAlt,
		GameConfigKeys::Key_Laser1NegAlt,
		GameConfigKeys::Key_Laser1PosAlt,
	};

	const Vector<GameConfigKeys> m_controllerKeys = {
		GameConfigKeys::Controller_BTS,
		GameConfigKeys::Controller_BT0,
		GameConfigKeys::Controller_BT1,
		GameConfigKeys::Controller_BT2,
		GameConfigKeys::Controller_BT3,
		GameConfigKeys::Controller_FX0,
		GameConfigKeys::Controller_FX1,
		GameConfigKeys::Controller_Back
	};
	const Vector<GameConfigKeys> m_controllerLaserKeys = {
		GameConfigKeys::Controller_Laser0Axis,
		GameConfigKeys::Controller_Laser1Axis,
	};

	void RenderContents() override
	{
		UpdateInputKeyBindingStatus();
		UpdateControllerInputNames();

		RenderKeyBindings();

		LayoutRowDynamic(1);

		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, "_______________________");
		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, " ");

		if (nk_button_label(m_nctx, "Calibrate Laser Sensitivity")) OpenCalibrateSensitivity();

		GameConfigKeys laserSensKey;
		switch (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice))
		{
		case InputDevice::Controller:
			laserSensKey = GameConfigKeys::Controller_Sensitivity;
			break;
		case InputDevice::Mouse:
			laserSensKey = GameConfigKeys::Mouse_Sensitivity;
			break;
		case InputDevice::Keyboard:
		default:
			laserSensKey = GameConfigKeys::Key_Sensitivity;
			break;
		}

		FloatSetting(laserSensKey, "Laser Sensitivity (%g):", 0, 20, 0.001f);
		EnumSetting<Enum_ButtonComboModeSettings>(GameConfigKeys::UseBackCombo, "Use 3xBT+Start = Back:");
		EnumSetting<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice, "Button input mode:");
		EnumSetting<Enum_InputDevice>(GameConfigKeys::LaserInputDevice, "Laser input mode:");
		EnumSetting<Enum_LaserAxisOption>(GameConfigKeys::InvertLaserInput, "Invert laser input:");

		if (m_gamePads.size() > 0)
		{
			SelectionSetting(GameConfigKeys::Controller_DeviceID, m_gamePads, "Selected Controller:");
		}

		IntSetting(GameConfigKeys::GlobalOffset, "Global Offset", -1000, 1000);
		IntSetting(GameConfigKeys::InputOffset, "Input Offset", -1000, 1000);

		if (nk_button_label(m_nctx, "Calibrate offsets")) {
			CalibrationScreen* cscreen = new CalibrationScreen(m_nctx);
			g_transition->TransitionTo(cscreen);
		}

		FloatSetting(GameConfigKeys::SongSelSensMult, "Song Select Sensitivity Multiplier", 0.0f, 20.0f, 0.1f);
		IntSetting(GameConfigKeys::InputBounceGuard, "Button Bounce Guard:", 0, 100);

		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, " ");

		EnumSetting<Enum_AbortMethod>(GameConfigKeys::RestartPlayMethod, "Restart with F5:");
		if (g_gameConfig.GetEnum<Enum_AbortMethod>(GameConfigKeys::RestartPlayMethod) == AbortMethod::Hold)
		{
			IntSetting(GameConfigKeys::RestartPlayHoldDuration, "Restart Hold Duration (ms):", 250, 10000, 250);
		}

		EnumSetting<Enum_AbortMethod>(GameConfigKeys::ExitPlayMethod, "Exit gameplay with Back:");
		if (g_gameConfig.GetEnum<Enum_AbortMethod>(GameConfigKeys::ExitPlayMethod) == AbortMethod::Hold)
		{
			IntSetting(GameConfigKeys::ExitPlayHoldDuration, "Exit Hold Duration (ms):", 250, 10000, 250);
		}

		ToggleSetting(GameConfigKeys::DisableNonButtonInputsDuringPlay, "Disable non-buttons during gameplay");

		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, " ");

		if (nk_tree_push(m_nctx, NK_TREE_NODE, "Laser Assist", NK_MINIMIZED))
		{
			FloatSetting(GameConfigKeys::LaserAssistLevel, "Base Laser Assist", 0.0f, 10.0f, 0.1f);
			FloatSetting(GameConfigKeys::LaserPunish, "Base Laser Punish", 0.0f, 10.0f, 0.1f);
			FloatSetting(GameConfigKeys::LaserChangeTime, "Direction Change Duration (ms)", 0.0f, 1000.0f, 1.0f);
			FloatSetting(GameConfigKeys::LaserChangeExponent, "Direction Change Curve Exponent", 0.0f, 10.0f, 0.1f);

			nk_tree_pop(m_nctx);
		}
	}

private:
	inline void RenderKeyBindings()
	{
		LayoutRowDynamic(3);
		if (nk_button_label(m_nctx, m_controllerLaserNames[0].c_str())) OpenLeftLaserBind();
		if (nk_button_label(m_nctx, m_controllerButtonNames[0].c_str())) OpenButtonBind((*m_activeBTKeys)[0]);
		if (nk_button_label(m_nctx, m_controllerLaserNames[1].c_str())) OpenRightLaserBind();

		LayoutRowDynamic(4);
		if (nk_button_label(m_nctx, m_controllerButtonNames[1].c_str())) OpenButtonBind((*m_activeBTKeys)[1]);
		if (nk_button_label(m_nctx, m_controllerButtonNames[2].c_str())) OpenButtonBind((*m_activeBTKeys)[2]);
		if (nk_button_label(m_nctx, m_controllerButtonNames[3].c_str())) OpenButtonBind((*m_activeBTKeys)[3]);
		if (nk_button_label(m_nctx, m_controllerButtonNames[4].c_str())) OpenButtonBind((*m_activeBTKeys)[4]);

		LayoutRowDynamic(2);
		if (nk_button_label(m_nctx, m_controllerButtonNames[5].c_str())) OpenButtonBind((*m_activeBTKeys)[5]);
		if (nk_button_label(m_nctx, m_controllerButtonNames[6].c_str())) OpenButtonBind((*m_activeBTKeys)[6]);

		if (!m_useBTGamepad)
		{
			if (!nk_option_label(m_nctx, "Primary", m_altBinds ? 1 : 0)) m_altBinds = false;
			if (!nk_option_label(m_nctx, "Alternate", m_altBinds ? 0 : 1)) m_altBinds = true;
		}

		LayoutRowDynamic(1);
		nk_label(m_nctx, "Back:", nk_text_alignment::NK_TEXT_LEFT);
		if (nk_button_label(m_nctx, m_controllerButtonNames[7].c_str())) OpenButtonBind((*m_activeBTKeys)[7]);
	}

	inline void OpenLeftLaserBind()
	{
		OpenLaserBind(GameConfigKeys::Controller_Laser0Axis);
	}
	inline void OpenRightLaserBind()
	{
		OpenLaserBind(GameConfigKeys::Controller_Laser1Axis);
	}
	inline void OpenLaserBind(GameConfigKeys axis)
	{
		const InputDevice laserInputDevice = g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice);
		OpenButtonBind(axis, laserInputDevice == InputDevice::Controller);
	}

	inline void OpenButtonBind(GameConfigKeys key)
	{
		OpenButtonBind(key, m_useBTGamepad);
	}
	inline void OpenButtonBind(GameConfigKeys key, bool gamepad)
	{
		g_application->AddTickable(ButtonBindingScreen::Create(key, gamepad, g_gameConfig.GetInt(GameConfigKeys::Controller_DeviceID), m_altBinds));
	}

	inline void OpenCalibrateSensitivity()
	{
		LaserSensCalibrationScreen* sensScreen = LaserSensCalibrationScreen::Create();
		sensScreen->SensSet.Add(this, &SettingsPage_Input::SetSensitivity);
		g_application->AddTickable(sensScreen);
	}
	inline void SetSensitivity(float sens)
	{
		switch (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice))
		{
		case InputDevice::Controller:
			g_gameConfig.Set(GameConfigKeys::Controller_Sensitivity, sens);
			break;
		case InputDevice::Mouse:
			g_gameConfig.Set(GameConfigKeys::Mouse_Sensitivity, sens);
			break;
		case InputDevice::Keyboard:
		default:
			g_gameConfig.Set(GameConfigKeys::Key_Sensitivity, sens);
			break;
		}
	}

	inline void UpdateInputKeyBindingStatus()
	{
		m_useBTGamepad = g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Controller;
		m_useLaserGamepad = g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice) == InputDevice::Controller;

		if (m_useBTGamepad) m_activeBTKeys = &m_controllerKeys;
		else if (m_altBinds) m_activeBTKeys = &m_altKeyboardKeys;
		else m_activeBTKeys = &m_keyboardKeys;

		if (m_altBinds) m_activeLaserKeys = &m_altKeyboardLaserKeys;
		else m_activeLaserKeys = &m_keyboardLaserKeys;
	}
	inline void UpdateControllerInputNames()
	{
		for (size_t i = 0; i < 8; i++)
		{
			if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Controller)
			{
				m_controllerButtonNames[i] = Utility::Sprintf("%d", g_gameConfig.GetInt(m_controllerKeys[i]));
			}
			else
			{
				m_controllerButtonNames[i] = GetKeyNameFromScancodeConfig(g_gameConfig.GetInt((*m_activeBTKeys)[i]));
			}
		}
		for (size_t i = 0; i < 2; i++)
		{
			if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice) == InputDevice::Controller)
			{
				m_controllerLaserNames[i] = Utility::Sprintf("%d", g_gameConfig.GetInt(m_controllerLaserKeys[i]));
			}
			else
			{
				m_controllerLaserNames[i] = Utility::ConvertToUTF8(Utility::WSprintf( // wstring->string because regular Sprintf messes up(?????)
					L"%ls / %ls",
					Utility::ConvertToWString(GetKeyNameFromScancodeConfig(g_gameConfig.GetInt((*m_activeLaserKeys)[i * 2]))),
					Utility::ConvertToWString(GetKeyNameFromScancodeConfig(g_gameConfig.GetInt((*m_activeLaserKeys)[i * 2 + 1])))
				));
			}
		}
	}
};

class SettingsPage_Game : public SettingsPage
{
public:
	SettingsPage_Game(nk_context* nctx) : SettingsPage(nctx, "Game") {}

protected:
	void Load() override
	{
		m_hitWindow = HitWindow::FromConfig();

		m_songsPath.Load();
	}

	void Save() override
	{
		m_hitWindow.Validate();
		m_hitWindow.SaveConfig();

		m_songsPath.Save();
	}

	HitWindow m_hitWindow = HitWindow::NORMAL;

	GameConfigTextData m_songsPath = { GameConfigKeys::SongFolder };

protected:
	void RenderContents() override
	{
		LayoutRowDynamic(1);

		EnumSetting<Enum_SpeedMods>(GameConfigKeys::SpeedMod, "Speed mod:");
		FloatSetting(GameConfigKeys::HiSpeed, "HiSpeed", 0.25f, 20, 0.05f);
		FloatSetting(GameConfigKeys::ModSpeed, "ModSpeed", 50, 1500, 0.5f);
		ToggleSetting(GameConfigKeys::AutoSaveSpeed, "Save hispeed changes during gameplay");

		IntSetting(GameConfigKeys::LeadInTime, "Lead-in time (ms)", 250, 10000, 250);
		IntSetting(GameConfigKeys::PracticeLeadInTime, "(for practice mode)", 250, 10000, 250);

		ToggleSetting(GameConfigKeys::PracticeSetupNavEnabled, "Enable navigation inputs for the practice setup");
		ToggleSetting(GameConfigKeys::RevertToSetupAfterScoreScreen, "Revert to the practice setup after the score screen is shown");

		ToggleSetting(GameConfigKeys::SkipScore, "Skip score screen on manual exit");
		EnumSetting<Enum_AutoScoreScreenshotSettings>(GameConfigKeys::AutoScoreScreenshot, "Automatically capture score screenshots:");

		{
			nk_label(m_nctx, "Timing Window:", nk_text_alignment::NK_TEXT_LEFT);
			LayoutRowDynamic(3);

			const int hitWindowPerfect = IntInput(m_hitWindow.perfect, "Crit", 0, HitWindow::NORMAL.perfect);
			if (hitWindowPerfect != m_hitWindow.perfect)
			{
				m_hitWindow.perfect = hitWindowPerfect;
				if (m_hitWindow.good < m_hitWindow.perfect)
					m_hitWindow.good = m_hitWindow.perfect;
				if (m_hitWindow.hold < m_hitWindow.perfect)
					m_hitWindow.hold = m_hitWindow.perfect;
			}

			const int hitWindowGood = IntInput(m_hitWindow.good, "Near", 0, HitWindow::NORMAL.good);
			if (hitWindowGood != m_hitWindow.good)
			{
				m_hitWindow.good = hitWindowGood;
				if (m_hitWindow.good < m_hitWindow.perfect)
					m_hitWindow.perfect = m_hitWindow.good;
				if (m_hitWindow.hold < m_hitWindow.good)
					m_hitWindow.hold = m_hitWindow.good;
			}

			const int hitWindowHold = IntInput(m_hitWindow.hold, "Hold", 0, HitWindow::NORMAL.hold);
			if (hitWindowHold != m_hitWindow.hold)
			{
				m_hitWindow.hold = hitWindowHold;
				if (m_hitWindow.hold < m_hitWindow.perfect)
					m_hitWindow.perfect = m_hitWindow.hold;
				if (m_hitWindow.hold < m_hitWindow.good)
					m_hitWindow.good = m_hitWindow.hold;
			}

			LayoutRowDynamic(2);

			if (nk_button_label(m_nctx, "Set to NORMAL (default)"))
			{
				m_hitWindow = HitWindow::NORMAL;
			}

			if (nk_button_label(m_nctx, "Set to HARD"))
			{
				m_hitWindow = HitWindow::HARD;
			}

			LayoutRowDynamic(1);
		}

		nk_label(m_nctx, "Songs folder path:", nk_text_alignment::NK_TEXT_LEFT);
		m_songsPath.Render(m_nctx);

		ToggleSetting(GameConfigKeys::TransferScoresOnChartUpdate, "Transfer scores on chart change");

		ToggleSetting(GameConfigKeys::AutoComputeSongOffset, "Auto-compute the song offset on first play");
	}
};

class SettingsPage_Display : public SettingsPage
{
public:
	SettingsPage_Display(nk_context* nctx) : SettingsPage(nctx, "Display") {}

protected:
	void Load() override
	{
		m_skins = Path::GetSubDirs(Path::Normalize(Path::Absolute("skins/")));

		m_laserColors[0] = g_gameConfig.GetFloat(GameConfigKeys::Laser0Color);
		m_laserColors[1] = g_gameConfig.GetFloat(GameConfigKeys::Laser1Color);
	}

	void Save() override
	{
		g_gameConfig.Set(GameConfigKeys::Laser0Color, m_laserColors[0]);
		g_gameConfig.Set(GameConfigKeys::Laser1Color, m_laserColors[1]);

		if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Mouse)
		{
			g_gameConfig.SetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice, InputDevice::Keyboard);
		}
	}
	
	Vector<String> m_skins;
	std::array<float, 2> m_laserColors = { 200.0f, 330.0f };

	void RenderContents() override
	{
		LayoutRowDynamic(1);
		ToggleSetting(GameConfigKeys::EnableHiddenSudden, "Enable Hidden / Sudden Mode");

		LayoutRowDynamic(2, 75);

		if (nk_group_begin(m_nctx, "Hidden", NK_WINDOW_NO_SCROLLBAR))
		{
			LayoutRowDynamic(1);
			FloatSetting(GameConfigKeys::HiddenCutoff, "Hidden Cutoff", 0.0f, 1.0f);
			FloatSetting(GameConfigKeys::HiddenFade, "Hidden Fade", 0.0f, 1.0f);
			nk_group_end(m_nctx);
		}

		if (nk_group_begin(m_nctx, "Sudden", NK_WINDOW_NO_SCROLLBAR))
		{
			LayoutRowDynamic(1);
			FloatSetting(GameConfigKeys::SuddenCutoff, "Sudden Cutoff", 0.0f, 1.0f);
			FloatSetting(GameConfigKeys::SuddenFade, "Sudden Fade", 0.0f, 1.0f);
			nk_group_end(m_nctx);
		}

		LayoutRowDynamic(1);
		ToggleSetting(GameConfigKeys::DisableBackgrounds, "Disable Song Backgrounds");
		FloatSetting(GameConfigKeys::DistantButtonScale, "Distant Button Scale", 1.0f, 5.0f);
		ToggleSetting(GameConfigKeys::ShowCover, "Show Track Cover");

		if (m_skins.size() > 0)
		{
			if (StringSelectionSetting(GameConfigKeys::Skin, m_skins, "Selected Skin:")) {
				// Window cursor
				Image cursorImg = ImageRes::Create(Path::Absolute("skins/" + g_gameConfig.GetString(GameConfigKeys::Skin) + "/textures/cursor.png"));
				g_gameWindow->SetCursor(cursorImg, Vector2i(5, 5));
			}
		}

		EnumSetting<Enum_ScoreDisplayModes>(GameConfigKeys::ScoreDisplayMode, "In-game score display is:");

		RenderLaserColorSetting();

		LayoutRowDynamic(1);
		ToggleSetting(GameConfigKeys::DisplayPracticeInfoInGame, "Show practice info during gameplay");
	}

private:
	const std::array<float, 4> m_laserColorPalette = { 330.f, 60.f, 100.f, 200.f };
	bool m_laserColorPaletteVisible = false;

	void RenderLaserColorSetting()
	{
		const nk_color leftColor = nk_hsv_f(m_laserColors[0] / 360, 1, 1);
		const nk_color rightColor = nk_hsv_f(m_laserColors[1] / 360, 1, 1);

		const int lcolInt = static_cast<int>(m_laserColors[0]);
		const int rcolInt = static_cast<int>(m_laserColors[1]);

		LayoutRowDynamic(1);
		nk_label(m_nctx, "Laser colors:", nk_text_alignment::NK_TEXT_LEFT);

		LayoutRowDynamic(2);

		// Color
		if (nk_button_color(m_nctx, leftColor)) m_laserColorPaletteVisible = !m_laserColorPaletteVisible;
		if (nk_button_color(m_nctx, rightColor)) m_laserColorPaletteVisible = !m_laserColorPaletteVisible;

		// Palette
		if (m_laserColorPaletteVisible)
		{
			LayoutRowDynamic(2 * static_cast<int>(m_laserColorPalette.size()));

			RenderLaserColorPalette(m_laserColors.data());
			RenderLaserColorPalette(m_laserColors.data() + 1);

			LayoutRowDynamic(2);
		}

		// Text
		{
			const int lcolIntNew = IntInput(lcolInt, "LLaser Hue", 0, 360);
			if (lcolIntNew != lcolInt) m_laserColors[0] = static_cast<float>(lcolIntNew);

			const int rcolIntNew = IntInput(rcolInt, "RLaser Hue", 0, 360);
			if (rcolIntNew != rcolInt) m_laserColors[1] = static_cast<float>(rcolIntNew);
		}

		// Slider
		{
			nk_slider_float(m_nctx, 0, m_laserColors.data(), 360, 0.1f);
			nk_slider_float(m_nctx, 0, m_laserColors.data() + 1, 360, 0.1f);
		}
	}

	void RenderLaserColorPalette(float* laserColor)
	{
		for (const float paletteHue : m_laserColorPalette)
		{
			const nk_color paletteColor = nk_hsv_f(paletteHue / 360, 1, 1);
			if (nk_button_color(m_nctx, paletteColor))
			{
				*laserColor = paletteHue;
			}
		}
	}
};

class SettingsPage_System : public SettingsPage
{
public:
	SettingsPage_System(nk_context* nctx) : SettingsPage(nctx, "System") {}

protected:
	void Load() override
	{
		m_channels = { "release", "master", "develop" };
		String channel = g_gameConfig.GetString(GameConfigKeys::UpdateChannel);

		if (!m_channels.Contains(channel))
		{
			m_channels.insert(m_channels.begin(), channel);
		}
	}

	void Save() override
	{
		if (g_gameConfig.GetBool(GameConfigKeys::CheckForUpdates))
		{
			g_application->CheckForUpdate();
		}
	}

	const Vector<const char*> m_aaModes = { "Off", "2x MSAA", "4x MSAA", "8x MSAA", "16x MSAA" };
	Vector<String> m_channels;

protected:
	void RenderContents() override
	{
		LayoutRowDynamic(1);

		PercentSetting(GameConfigKeys::MasterVolume, "Master Volume (%.1f%%):");
		ToggleSetting(GameConfigKeys::WindowedFullscreen, "Use windowed fullscreen");
		ToggleSetting(GameConfigKeys::ForcePortrait, "Force portrait rendering (don't use if already in portrait)");
		ToggleSetting(GameConfigKeys::VSync, "VSync");
		ToggleSetting(GameConfigKeys::ShowFps, "Show FPS");

		SelectionSetting(GameConfigKeys::AntiAliasing, m_aaModes, "Anti-aliasing (requires restart):");

#ifdef _WIN32
		ToggleSetting(GameConfigKeys::WASAPI_Exclusive, "WASAPI Exclusive Mode (requires restart)");
#endif // _WIN32

		ToggleSetting(GameConfigKeys::MuteUnfocused, "Mute the game when unfocused");
		ToggleSetting(GameConfigKeys::PrerenderEffects, "Pre-Render Song Effects (experimental)");
		ToggleSetting(GameConfigKeys::CheckForUpdates, "Check for updates on startup");

		if (m_channels.size() > 0)
		{
			StringSelectionSetting(GameConfigKeys::UpdateChannel, m_channels, "Update Channel:");
		}

		EnumSetting<Logger::Enum_Severity>(GameConfigKeys::LogLevel, "Logging level");
	}
};

class SettingsPage_Online : public SettingsPage
{
public:
	SettingsPage_Online(nk_context* nctx) : SettingsPage(nctx, "Online") {}
	
protected:
	void Load() override
	{
		m_multiplayerHost.Load();
		m_multiplayerPassword.Load();
		m_multiplayerUsername.Load();

		m_irBaseURL.Load();
		m_irToken.Load();
	}

	void Save() override
	{
		m_multiplayerHost.Save();
		m_multiplayerPassword.Save();
		m_multiplayerUsername.Save();

		m_irBaseURL.Save();
		m_irToken.Save();
	}

	GameConfigTextData m_multiplayerHost = { GameConfigKeys::MultiplayerHost };
	GameConfigTextData m_multiplayerPassword = { GameConfigKeys::MultiplayerPassword };
	GameConfigTextData m_multiplayerUsername = { GameConfigKeys::MultiplayerUsername };

	GameConfigTextData m_irBaseURL = { GameConfigKeys::IRBaseURL };
	GameConfigTextData m_irToken = { GameConfigKeys::IRToken };

	void RenderContents() override
	{
		LayoutRowDynamic(1);

		nk_label(m_nctx, "Multiplayer Server:", nk_text_alignment::NK_TEXT_LEFT);
		m_multiplayerHost.Render(m_nctx);

		nk_label(m_nctx, "Multiplayer Server Username:", nk_text_alignment::NK_TEXT_LEFT);
		m_multiplayerUsername.Render(m_nctx);

		nk_label(m_nctx, "Multiplayer Server Password:", nk_text_alignment::NK_TEXT_LEFT);
		m_multiplayerPassword.RenderPassword(m_nctx);

		nk_label(m_nctx, "IR Base URL:", nk_text_alignment::NK_TEXT_LEFT);
		m_irBaseURL.Render(m_nctx);

		nk_label(m_nctx, "IR Token:", nk_text_alignment::NK_TEXT_LEFT);
		m_irToken.RenderPassword(m_nctx);

		ToggleSetting(GameConfigKeys::IRLowBandwidth, "IR Low Bandwidth (disables sending replays)");
	}
};

class SettingsPage_Skin : public SettingsPage
{
public:
	SettingsPage_Skin(nk_context* nctx) : SettingsPage(nctx, "Skin") {}

protected:
	void Load() override
	{
		m_skin = g_gameConfig.GetString(GameConfigKeys::Skin);
		if (m_skin == g_application->GetCurrentSkin())
		{
			m_skinConfig = g_skinConfig;
		}
		else
		{
			m_skinConfig = new SkinConfig(m_skin);
		}

		m_allSkins = Path::GetSubDirs(Path::Normalize(Path::Absolute("skins/")));
		m_skinConfigTextData.clear();
		m_useHSVMap.clear();
	}

	void Save() override
	{
		if (m_skinConfig != g_skinConfig && m_skinConfig)
		{
			delete m_skinConfig;
			m_skinConfig = nullptr;
		}

		for (auto& it : m_skinConfigTextData)
		{
			it.second.Save();
		}
	}

	String m_skin;
	SkinConfig* m_skinConfig = nullptr;

	Vector<String> m_allSkins;

	void RenderContents() override
	{
		LayoutRowDynamic(1);

		if (SkinSelectionSetting("Selected Skin:"))
		{
			Image cursorImg = ImageRes::Create(Path::Absolute("skins/" + g_gameConfig.GetString(GameConfigKeys::Skin) + "/textures/cursor.png"));
			g_gameWindow->SetCursor(cursorImg, Vector2i(5, 5));
		}

		if (m_skinConfig == nullptr)
		{
			return;
		}

		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, "%s Skin Settings", m_skin.data());
		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, "_______________________");

		for (const SkinSetting& setting : m_skinConfig->GetSettings())
		{
			RenderSkinSetting(setting);
		}
	}

private:
	class SkinConfigTextData : public SettingTextData
	{
	public:
		SkinConfigTextData(SkinConfig* skinConfig, const String& key) : m_skinConfig(skinConfig), m_key(key) {}

	protected:
		String LoadConfig() override { return m_skinConfig ? m_skinConfig->GetString(m_key) : ""; }
		void SaveConfig(const String& value) override { if(m_skinConfig) m_skinConfig->Set(m_key, value); }

		SkinConfig* m_skinConfig = nullptr;
		String m_key;
	};

	Map<String, SkinConfigTextData> m_skinConfigTextData;
	Map<String, bool> m_useHSVMap;

	bool SkinSelectionSetting(const std::string_view& label)
	{
		if (m_allSkins.empty())
		{
			return false;
		}

		if (StringSelectionSetting(GameConfigKeys::Skin, m_allSkins, label))
		{
			Save(); Load();
			return true;
		}

		return false;
	}

	inline bool SkinToggleSetting(const SkinSetting& setting)
	{
		const bool value = m_skinConfig->GetBool(setting.key);
		const bool newValue = ToggleInput(value, setting.label);

		if (newValue != value)
		{
			m_skinConfig->Set(setting.key, newValue);
			return true;
		}

		return false;
	}

	inline bool SkinStringSelectionSetting(const SkinSetting& setting)
	{
		if (setting.selectionSetting.numOptions == 0)
		{
			return false;
		}

		const String& value = m_skinConfig->GetString(setting.key);
		int selection = 0;

		const String* options = setting.selectionSetting.options;
		const auto stringSearch = std::find(options, options + setting.selectionSetting.numOptions, value);
		if (stringSearch != options + setting.selectionSetting.numOptions)
		{
			selection = static_cast<int>(stringSearch - options);
		}

		Vector<const char*> displayData;
		for (int i=0; i<setting.selectionSetting.numOptions; ++i)
		{
			displayData.Add(setting.selectionSetting.options[i].data());
		}

		const int newSelection = SelectionInput(selection, displayData, setting.label);

		if (newSelection != selection) {
			m_skinConfig->Set(setting.key, options[newSelection]);
			return true;
		}

		return false;
	}

	inline bool SkinIntSetting(const SkinSetting& setting)
	{
		const int value = m_skinConfig->GetInt(setting.key);
		const int newValue = IntInput(value, setting.label,
			setting.intSetting.min, setting.intSetting.max);

		if (newValue != value) {
			m_skinConfig->Set(setting.key, newValue);
			return true;
		}

		return false;
	}

	inline bool SkinFloatSetting(const SkinSetting& setting)
	{
		float value = m_skinConfig->GetFloat(setting.key);
		const float prevValue = value;
		const float step = 0.01f;

		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_LEFT, setting.label.data(), value);
		nk_slider_float(m_nctx, setting.floatSetting.min, &value, setting.floatSetting.max, step);
		
		if (prevValue != value) {
			m_skinConfig->Set(setting.key, value);
			return true;
		}

		return false;
	}

	inline void SkinTextSetting(const SkinSetting& setting)
	{
		auto it = m_skinConfigTextData.find(setting.key);
		if (it == m_skinConfigTextData.end())
		{
			it = m_skinConfigTextData.emplace_hint(it, setting.key, SkinConfigTextData { m_skinConfig, setting.key });
		}

		if (it == m_skinConfigTextData.end())
		{
			return;
		}


		nk_label(m_nctx, setting.label.data(), nk_text_alignment::NK_TEXT_LEFT);

		SkinConfigTextData& data = it->second;

		if (setting.textSetting.secret)
		{
			data.RenderPassword(m_nctx);
		}
		else
		{
			data.Render(m_nctx);
		}
	}

	inline bool SkinColorSetting(const SkinSetting& setting)
	{
		const Color value = m_skinConfig->GetColor(setting.key);
		const Color newValue = ColorInput(value, setting.label, m_useHSVMap[setting.key]);

		if (newValue != value)
		{
			m_skinConfig->Set(setting.key, newValue);
			return true;
		}

		return false;
	}

	void RenderSkinSetting(const SkinSetting& setting)
	{
		switch (setting.type)
		{
		case SkinSetting::Type::Separator:
			Separator();
			break;
		case SkinSetting::Type::Label:
			Label(setting.key);
			break;
		case SkinSetting::Type::Boolean:
			SkinToggleSetting(setting);
			break;
		case SkinSetting::Type::Selection:
			SkinStringSelectionSetting(setting);
			break;
		case SkinSetting::Type::Integer:
			SkinIntSetting(setting);
			break;
		case SkinSetting::Type::Float:
			SkinFloatSetting(setting);
			break;
		case SkinSetting::Type::Text:
			SkinTextSetting(setting);
			break;
		case SkinSetting::Type::Color:
			SkinColorSetting(setting);
			break;
		}
	}
};

class SettingsScreen_Impl : public SettingsPageCollection
{
public:
	void OnSuspend() override
	{
	}

	void OnRestore() override
	{
		g_application->DiscordPresenceMenu("Settings");
	}

protected:
	void AddPages(Vector<std::unique_ptr<SettingsPage>>& pages) override
	{
		pages.emplace_back(std::make_unique<SettingsPage_Input>(m_nctx));
		pages.emplace_back(std::make_unique<SettingsPage_Game>(m_nctx));
		pages.emplace_back(std::make_unique<SettingsPage_Display>(m_nctx));
		pages.emplace_back(std::make_unique<SettingsPage_System>(m_nctx));
		pages.emplace_back(std::make_unique<SettingsPage_Online>(m_nctx));
		pages.emplace_back(std::make_unique<SettingsPage_Skin>(m_nctx));
	}
};

IApplicationTickable* SettingsScreen::Create()
{
	return new SettingsScreen_Impl();
}

class ButtonBindingScreen_Impl : public ButtonBindingScreen
{
private:
	Ref<Gamepad> m_gamepad;
	//Label* m_prompt;
	GameConfigKeys m_key;
	bool m_isGamepad;
	int m_gamepadIndex;
	bool m_completed = false;
	bool m_knobs = false;
	bool m_isAlt = false;
	Vector<float> m_gamepadAxes;

public:
	ButtonBindingScreen_Impl(GameConfigKeys key, bool gamepad, int controllerIndex, bool isAlt)
	{
		m_key = key;
		m_gamepadIndex = controllerIndex;
		m_isGamepad = gamepad;
		m_knobs = (key == GameConfigKeys::Controller_Laser0Axis || key == GameConfigKeys::Controller_Laser1Axis);
		m_isAlt = isAlt;
	}

	bool Init()
	{
		if (m_isGamepad)
		{
			m_gamepad = g_gameWindow->OpenGamepad(m_gamepadIndex);
			if (!m_gamepad)
			{
				Logf("Failed to open gamepad: %d", Logger::Severity::Error, m_gamepadIndex);
				g_gameWindow->ShowMessageBox("Warning", "Could not open selected gamepad.\nEnsure the controller is connected and in the correct mode (if applicable) and selected in the previous menu.", 1);
				return false;
			}
			if (m_knobs)
			{
				for (size_t i = 0; i < m_gamepad->NumAxes(); i++)
				{
					m_gamepadAxes.Add(m_gamepad->GetAxis(i));
				}
			}
			else
			{
				m_gamepad->OnButtonPressed.Add(this, &ButtonBindingScreen_Impl::OnButtonPressed);
			}
		}
		return true;
	}

	void Tick(float deltatime)
	{
		if (m_knobs && m_isGamepad)
		{
			for (uint8 i = 0; i < m_gamepad->NumAxes(); i++)
			{
				float delta = fabs(m_gamepad->GetAxis(i) - m_gamepadAxes[i]);
				if (delta > 0.3f)
				{
					g_gameConfig.Set(m_key, i);
					m_completed = true;
					break;
				}

			}
		}

		if (m_completed && m_gamepad)
		{
			m_gamepad->OnButtonPressed.RemoveAll(this);
			m_gamepad.reset();

			g_application->RemoveTickable(this);
		}
		else if (m_completed && !m_knobs)
		{
			g_application->RemoveTickable(this);
		}

	}

	void Render(float deltatime)
	{
		String prompt = "Press Key";

		if (m_knobs)
		{
			prompt = "Press Left Key";
			if (m_completed)
			{
				prompt = "Press Right Key";
			}
		}

		if (m_isGamepad)
		{
			prompt = "Press Button";
			m_gamepad = g_gameWindow->OpenGamepad(m_gamepadIndex);
			if (m_knobs)
			{
				prompt = "Turn Knob";
				for (size_t i = 0; i < m_gamepad->NumAxes(); i++)
				{
					m_gamepadAxes.Add(m_gamepad->GetAxis(i));
				}
			}
		}

		g_application->FastText(prompt, g_resolution.x / 2, g_resolution.y / 2, 40, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);
	}

	void OnButtonPressed(uint8 key)
	{
		if (!m_knobs)
		{
			g_gameConfig.Set(m_key, key);
			m_completed = true;
		}
	}

	virtual void OnKeyPressed(SDL_Scancode code)
	{
		if (!m_isGamepad && !m_knobs)
		{
			g_gameConfig.Set(m_key, code);
			m_completed = true; // Needs to be set because pressing right alt triggers two keypresses on the same frame.
		}
		else if (!m_isGamepad && m_knobs)
		{
			switch (m_key)
			{
			case GameConfigKeys::Controller_Laser0Axis:
				g_gameConfig.Set(m_completed ?
					m_isAlt ? GameConfigKeys::Key_Laser0PosAlt : GameConfigKeys::Key_Laser0Pos :
					m_isAlt ? GameConfigKeys::Key_Laser0NegAlt : GameConfigKeys::Key_Laser0Neg,
					code);
				break;
			case GameConfigKeys::Controller_Laser1Axis:
				g_gameConfig.Set(m_completed ?
					m_isAlt ? GameConfigKeys::Key_Laser1PosAlt : GameConfigKeys::Key_Laser1Pos :
					m_isAlt ? GameConfigKeys::Key_Laser1NegAlt : GameConfigKeys::Key_Laser1Neg,
					code);
				break;
			default:
				break;
			}

			if (!m_completed)
			{
				m_completed = true;
			}
			else
			{
				g_application->RemoveTickable(this);
			}
		}
	}

	virtual void OnSuspend()
	{
		//g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		//Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		//slot->anchor = Anchors::Full;
	}
};

ButtonBindingScreen* ButtonBindingScreen::Create(GameConfigKeys key, bool gamepad, int controllerIndex, bool isAlternative)
{
	ButtonBindingScreen_Impl* impl = new ButtonBindingScreen_Impl(key, gamepad, controllerIndex, isAlternative);
	return impl;
}

class LaserSensCalibrationScreen_Impl : public LaserSensCalibrationScreen
{
private:
	Ref<Gamepad> m_gamepad;
	//Label* m_prompt;
	bool m_state = false;
	float m_delta = 0.f;
	float m_currentSetting = 0.f;
	bool m_firstStart = false;
public:
	LaserSensCalibrationScreen_Impl()
	{

	}

	~LaserSensCalibrationScreen_Impl()
	{
		g_input.OnButtonPressed.RemoveAll(this);
	}

	bool Init()
	{
		g_input.GetInputLaserDir(0); //poll because there might be something idk

		if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice) == InputDevice::Controller)
			m_currentSetting = g_gameConfig.GetFloat(GameConfigKeys::Controller_Sensitivity);
		else
			m_currentSetting = g_gameConfig.GetFloat(GameConfigKeys::Mouse_Sensitivity);

		g_input.OnButtonPressed.Add(this, &LaserSensCalibrationScreen_Impl::OnButtonPressed);
		return true;
	}

	void Tick(float deltatime)
	{
		m_delta += g_input.GetAbsoluteInputLaserDir(0);

	}

	void Render(float deltatime)
	{
		if (m_state)
		{
			float sens = 6.0 / m_delta;

			g_application->FastText("Turn left knob one revolution clockwise", g_resolution.x / 2, g_resolution.y / 2, 40, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);
			g_application->FastText("then press start.", g_resolution.x / 2, g_resolution.y / 2 + 45, 40, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);
			g_application->FastText(Utility::Sprintf("Current Sens: %.2f", sens), g_resolution.x / 2, g_resolution.y / 2 + 90, 40, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);

		}
		else
		{
			m_delta = 0;
			g_application->FastText("Press start twice", g_resolution.x / 2, g_resolution.y / 2, 40, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);
		}
	}

	void OnButtonPressed(Input::Button button)
	{
		if (button == Input::Button::BT_S)
		{
			if (m_firstStart)
			{
				if (m_state)
				{
					// calc sens and then call delagate
					SensSet.Call(6.0 / m_delta);
					g_application->RemoveTickable(this);
				}
				else
				{
					m_delta = 0;
					m_state = !m_state;
				}
			}
			else
			{
				m_firstStart = true;
			}
		}
	}

	virtual void OnKeyPressed(SDL_Scancode code)
	{
		if (code == SDL_SCANCODE_ESCAPE)
			g_application->RemoveTickable(this);
	}

	virtual void OnSuspend()
	{
		//g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		//Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		//slot->anchor = Anchors::Full;
	}
};

LaserSensCalibrationScreen* LaserSensCalibrationScreen::Create()
{
	LaserSensCalibrationScreen* impl = new LaserSensCalibrationScreen_Impl();
	return impl;
}

