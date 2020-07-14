#pragma once
#include "BaseGameSettingsDialog.hpp"
#include "Beatmap/BeatmapObjects.hpp"

class PracticeModeSettingsDialog : public BaseGameSettingsDialog
{
public:
	PracticeModeSettingsDialog(MapTime endTime, MapTimeRange& range);
	void InitTabs() override;

private:
	Tab m_CreatePlaybackTab();
	Tab m_CreateConditionTab();

	MapTime m_endTime;
	MapTimeRange& m_range;

	SettingData* m_loopBegin = nullptr;
	SettingData* m_loopEnd = nullptr;
	SettingData* m_speed = nullptr;
};

