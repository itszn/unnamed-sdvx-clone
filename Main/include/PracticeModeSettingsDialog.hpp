#pragma once
#include "BaseGameSettingsDialog.hpp"
#include "Beatmap/BeatmapObjects.hpp"
#include "Game.hpp"

class PracticeModeSettingsDialog : public BaseGameSettingsDialog
{
public:
	PracticeModeSettingsDialog(MapTime endTime, MapTime& lastMapTime, Game::PlayOptions& playOptions, MapTimeRange& range);
	void InitTabs() override;

	Delegate<MapTime> onSetMapTime;
	Delegate<float> onSpeedChange;
	Delegate<> onSettingChange;

private:
	Tab m_CreatePlaybackTab();
	Tab m_CreateFailConditionTab();
	Tab m_CreateGameSettingTab();

	MapTime m_endTime;
	MapTime& m_lastMapTime;

	// for ranges, use m_range instead of m_playOptions
	MapTimeRange& m_range;
	Game::PlayOptions& m_playOptions;
};

