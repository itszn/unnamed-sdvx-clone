#pragma once
#include "Shared/Timer.hpp"
#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Audio/Sample.hpp"
#include "Beatmap/BeatmapPlayback.hpp"

class CalibrationScreen: public IAsyncLoadableApplicationTickable
{
public:
	CalibrationScreen(struct nk_context* nk_ctx);
	bool AsyncLoad() override;
	bool AsyncFinalize() override;
	void Render(float deltaTime) override;
	void Tick(float deltaTime) override;
	void OnKeyPressed(int32 key) override;
	void OnKeyReleased(int32 key) override;

private:
	Timer m_timer;
	Track m_track;
	Camera m_camera;
	Sample m_metronome;
	BeatmapPlayback m_playback;
	float m_hispeed = 2.0;
	int m_audioOffset;
	int m_inputOffset;
	MapTime m_lastTime;
	struct nk_context* m_ctx;

	void m_OnButtonPressed(Input::Button buttonCode);
	void m_OnButtonReleased(Input::Button buttonCode);
};