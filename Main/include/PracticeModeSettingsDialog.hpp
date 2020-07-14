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

private:
	Tab m_CreatePlaybackTab();
	Tab m_CreateConditionTab();

	MapTime m_endTime;
	MapTime& m_lastMapTime;

	// for ranges, use m_range instead of m_playOptions
	MapTimeRange& m_range;
	Game::PlayOptions& m_playOptions;
};

