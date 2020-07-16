#pragma once
#include "BaseGameSettingsDialog.hpp"
#include "Beatmap/BeatmapObjects.hpp"
#include "Game.hpp"

class PracticeModeSettingsDialog : public BaseGameSettingsDialog
{
public:
	PracticeModeSettingsDialog(Ref<Beatmap> beatmap, MapTime& lastMapTime, Game::PlayOptions& playOptions, MapTimeRange& range);
	void InitTabs() override;

	Delegate<MapTime> onSetMapTime;
	Delegate<float> onSpeedChange;
	Delegate<> onSettingChange;

private:
	Tab m_CreatePlaybackTab();
	Tab m_CreateFailConditionTab();
	Tab m_CreateGameSettingTab();

	inline MapTime m_MeasureToTime(int measure) const { return m_beatmap->GetMapTimeFromMeasureInd(measure-1); }
	inline int m_TimeToMeasure(MapTime time) const { return m_beatmap->GetMeasureIndFromMapTime(time)+1; }

	Ref<Beatmap> m_beatmap;
	MapTime m_endTime;
	MapTime& m_lastMapTime;

	// for ranges, use m_range instead of m_playOptions
	MapTimeRange& m_range;
	Game::PlayOptions& m_playOptions;

	// Offset by 1
	int m_startMeasure = 1;
	int m_endMeasure = 1;
};

