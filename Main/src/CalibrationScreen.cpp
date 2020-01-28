#include "stdafx.h"
#include "Application.hpp"
#include "CalibrationScreen.hpp"
#include "Audio/Audio.hpp"
#include "SDL2/SDL_keycode.h"
#include "SettingsScreen.hpp"
#include "../third_party/nuklear/nuklear.h"


CalibrationScreen::CalibrationScreen(nk_context* nk_ctx)
{
	m_ctx = nk_ctx;
}

bool CalibrationScreen::AsyncLoad()
{
	m_metronome = SampleRes::Create(g_audio, Path::Normalize(Path::Absolute("audio/metronome120.wav")));
	m_playback.MakeCalibrationPlayback();
	m_audioOffset = g_gameConfig.GetInt(GameConfigKeys::GlobalOffset);
	m_inputOffset = g_gameConfig.GetInt(GameConfigKeys::InputOffset);
	return m_track.AsyncLoad();
}

bool CalibrationScreen::AsyncFinalize()
{
	m_track.suddenCutoff = g_gameConfig.GetFloat(GameConfigKeys::SuddenCutoff);
	m_track.suddenFadewindow = g_gameConfig.GetFloat(GameConfigKeys::SuddenFade);
	m_track.hiddenCutoff = g_gameConfig.GetFloat(GameConfigKeys::HiddenCutoff);
	m_track.hiddenFadewindow = g_gameConfig.GetFloat(GameConfigKeys::HiddenFade);

	m_timer.Restart();
	m_metronome->Play(true);
	m_camera.track = &m_track;
	return m_track.AsyncFinalize();
}

void CalibrationScreen::Render(float deltaTime)
{
	m_track.SetViewRange(8.0f / m_hispeed);

	RenderState rs = m_camera.CreateRenderState(true);
	RenderQueue renderQueue(g_gl, rs);

	MapTime msViewRange = m_playback.ViewDistanceToDuration(m_track.GetViewRange());
	auto currentObjectSet = m_playback.GetObjectsInRange(msViewRange);

	m_track.DrawBase(renderQueue);
	for (auto& object : currentObjectSet)
	{
		m_track.DrawObjectState(renderQueue, m_playback, object, false);
	}
	renderQueue.Process();

	//Draw nuklear GUI
	{
		if (nk_begin(m_ctx, "Options", nk_rect(50, 50, 400, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE))
		{
			nk_layout_row_dynamic(m_ctx, 30, 1);
			m_audioOffset = nk_propertyi(m_ctx, "Global Offset", -1000, m_audioOffset, 1000, 1, 1);
			m_inputOffset = nk_propertyi(m_ctx, "Input Offset", -1000, m_inputOffset, 1000, 1, 1);
			nk_label(m_ctx, "HiSpeed:", nk_text_alignment::NK_TEXT_LEFT);
			nk_slider_float(m_ctx, 0.5, &m_hispeed, 10.0f, 0.05);

			nk_layout_row_dynamic(m_ctx, 150, 2);
			if (nk_group_begin(m_ctx, "Hidden", NK_WINDOW_NO_SCROLLBAR))
			{
				nk_layout_row_dynamic(m_ctx, 30, 1);
				nk_label(m_ctx, "Hidden Cutoff:", nk_text_alignment::NK_TEXT_LEFT);
				nk_slider_float(m_ctx, 0.0f, &m_track.hiddenCutoff, 1.0f, 0.05f);
				nk_label(m_ctx, "Hidden Fade:", nk_text_alignment::NK_TEXT_LEFT);
				nk_slider_float(m_ctx, 0.0f, &m_track.hiddenFadewindow, 1.0f, 0.05f);
				nk_group_end(m_ctx);
			}
			if (nk_group_begin(m_ctx, "Sudden", NK_WINDOW_NO_SCROLLBAR))
			{
				nk_layout_row_dynamic(m_ctx, 30, 1);
				nk_label(m_ctx, "Sudden Cutoff:", nk_text_alignment::NK_TEXT_LEFT);
				nk_slider_float(m_ctx, 0.0f, &m_track.suddenCutoff, 1.0f, 0.05f);
				nk_label(m_ctx, "Sudden Fade:", nk_text_alignment::NK_TEXT_LEFT);
				nk_slider_float(m_ctx, 0.0f, &m_track.suddenFadewindow, 1.0f, 0.05f);
				nk_group_end(m_ctx);
			}

			nk_layout_row_dynamic(m_ctx, 30, 2);
			if (nk_button_label(m_ctx, "Cancel")) {
				g_application->RemoveTickable(this);
			}
			if (nk_button_label(m_ctx, "Ok")) {
				//TODO: Save settings
				g_application->RemoveTickable(this);
			}
			nk_end(m_ctx);
		}
		SettingsScreen::NKRender();
	}
}

void CalibrationScreen::Tick(float deltaTime)
{
	m_lastTime = m_timer.Milliseconds();
	m_lastTime = (1000 * m_metronome->GetPosition()) / m_metronome->GetSampleRate();
	m_lastTime -= m_audioOffset;


	m_playback.Update(m_lastTime);
	m_track.Tick(m_playback, deltaTime);
	m_camera.Tick(deltaTime, m_playback);
}

void CalibrationScreen::OnKeyPressed(int32 key)
{
	if (key == SDLK_ESCAPE)
	{
		g_application->RemoveTickable(this);
	}
}

void CalibrationScreen::OnKeyReleased(int32 key)
{
}
