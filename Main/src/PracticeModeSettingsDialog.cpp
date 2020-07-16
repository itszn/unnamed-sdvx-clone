#include "stdafx.h"
#include "PracticeModeSettingsDialog.hpp"
#include "Beatmap/MapDatabase.hpp"

PracticeModeSettingsDialog::PracticeModeSettingsDialog(Game& game, MapTime& lastMapTime,
    int32& tempOffset, Game::PlayOptions& playOptions, MapTimeRange& range)
    : m_chartIndex(game.GetChartIndex()), m_beatmap(game.GetBeatmap()),
    m_endTime(m_beatmap->GetLastObjectTime()),  m_lastMapTime(lastMapTime),
    m_tempOffset(tempOffset), m_playOptions(playOptions), m_range(range)
{
}

void PracticeModeSettingsDialog::InitTabs()
{
    AddTab(std::move(m_CreateMainSettingTab()));
    AddTab(std::move(m_CreateLoopingTab()));
    AddTab(std::move(m_CreateFailConditionTab()));
    AddTab(std::move(m_CreateOffsetTab()));

    SetCurrentTab(0);
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateMainSettingTab()
{
    Tab mainSettingTab = std::make_unique<TabData>();
    mainSettingTab->name = "Main";

    Setting loopBeginButton = CreateButton("Set to here (0ms)", [this](const auto&) {
        m_SetStartTime(Math::Clamp(m_lastMapTime, 0, m_endTime));
    });
    m_setStartButton = loopBeginButton.get();
    mainSettingTab->settings.emplace_back(std::move(loopBeginButton));

    Setting loopEndButton = CreateButton("Set to here (0ms)", [this](const auto&) {
        m_SetEndTime(m_lastMapTime);
    });
    m_setEndButton = loopEndButton.get();
    mainSettingTab->settings.emplace_back(std::move(loopEndButton));

    Setting loopOnSuccess = CreateBoolSetting("Loop on success", m_playOptions.loopOnSuccess);
    mainSettingTab->settings.emplace_back(std::move(loopOnSuccess));

    Setting loopOnFail = CreateBoolSetting("Loop on fail", m_playOptions.loopOnFail);
    mainSettingTab->settings.emplace_back(std::move(loopOnFail));

    Setting speedSetting = std::make_unique<SettingData>("Playback speed (%)", SettingType::Integer);
    speedSetting->intSetting.min = 25;
    speedSetting->intSetting.max = 100;
    speedSetting->intSetting.val = Math::Round(m_playOptions.playbackSpeed * 100);
    speedSetting->setter.AddLambda([this](const SettingData& data) { onSpeedChange.Call(data.intSetting.val == 100 ? 1.0f : data.intSetting.val / 100.0f); });
    speedSetting->getter.AddLambda([this](SettingData& data) { data.enumSetting.val = Math::Round(m_playOptions.playbackSpeed * 100); });
    mainSettingTab->settings.emplace_back(std::move(speedSetting));

    mainSettingTab->settings.emplace_back(CreateButton("Start practice", [this](const auto&) { onPressStart.Call(); }));

    return mainSettingTab;
}

void PracticeModeSettingsDialog::m_SetStartTime(MapTime time, int measure)
{
    m_range.begin = time;
    m_startMeasure = measure >= 0 ? measure : m_TimeToMeasure(time);
    m_setStartButton->name = Utility::Sprintf("Set to here (%dms)", time);
    onSetMapTime.Call(time);
    if (m_range.end < time)
    {
        m_range.end = time;
        m_endMeasure = m_startMeasure;
    }
}

void PracticeModeSettingsDialog::m_SetEndTime(MapTime time, int measure)
{
    m_range.end = time;
    m_endMeasure = measure >= 0 ? measure : m_TimeToMeasure(time);
    m_setEndButton->name = Utility::Sprintf("Set to here (%dms)", time);

    if(time != 0) onSetMapTime.Call(time);
}


PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateLoopingTab()
{
    Tab playbackTab = std::make_unique<TabData>();
    playbackTab->name = "Looping";

    // Loop begin
    {
        Setting loopBeginMeasureSetting = CreateIntSetting("Start point (measure #)", m_startMeasure, {1, m_TimeToMeasure(m_endTime)});
        loopBeginMeasureSetting->setter.AddLambda([this](const SettingData& data) {
            m_SetStartTime(m_MeasureToTime(data.intSetting.val), data.intSetting.val);
        });
        playbackTab->settings.emplace_back(std::move(loopBeginMeasureSetting));

        Setting loopBeginMSSetting = CreateIntSetting("- in milliseconds", m_range.begin, {0, m_endTime}, 50);
        loopBeginMSSetting->setter.AddLambda([this](const SettingData& data) {
            m_SetStartTime(data.intSetting.val);
        });
        playbackTab->settings.emplace_back(std::move(loopBeginMSSetting));

        Setting loopBeginButton = CreateButton("Set to here", [this](const auto&) {
            m_SetStartTime(Math::Clamp(m_lastMapTime, 0, m_endTime));
        });
        playbackTab->settings.emplace_back(std::move(loopBeginButton));
    }

    // Loop end
    {
        Setting loopEndMeasureSetting = CreateIntSetting("End point (measure #)", m_endMeasure, {1, m_TimeToMeasure(m_endTime)});
        loopEndMeasureSetting->setter.AddLambda([this](const SettingData& data) {
            m_SetEndTime(m_MeasureToTime(data.intSetting.val), data.intSetting.val);
        });
        playbackTab->settings.emplace_back(std::move(loopEndMeasureSetting));

        Setting loopEndMSSetting = CreateIntSetting("- in milliseconds", m_range.end, {0, m_endTime}, 50);
        loopEndMSSetting->setter.AddLambda([this](const SettingData& data) {
            m_SetEndTime(data.intSetting.val);
        });
        playbackTab->settings.emplace_back(std::move(loopEndMSSetting));

        Setting loopEndClearButton = CreateButton("Clear", [this](const auto&) {
            m_SetEndTime(0);
        });
        playbackTab->settings.emplace_back(std::move(loopEndClearButton));

        Setting loopEndButton = CreateButton("Set to here", [this](const auto&) {
            m_SetEndTime(m_lastMapTime);
        });
        playbackTab->settings.emplace_back(std::move(loopEndButton));
    }

    Setting loopOnSuccess = CreateBoolSetting("Loop on success", m_playOptions.loopOnSuccess);
    playbackTab->settings.emplace_back(std::move(loopOnSuccess));

    Setting loopOnFail = CreateBoolSetting("Loop on fail", m_playOptions.loopOnFail);
    playbackTab->settings.emplace_back(std::move(loopOnFail));

    return playbackTab;
}

enum class GameFailConditionType
{
    None, Score, Grade, Miss, MissAndNear
};

const char* GAME_FAIL_CONDITION_TYPE_STR[5] = { "None", "Score", "Grade", "Miss", "MissNear" };

static inline GameFailConditionType GetGameFailConditionType(const GameFailCondition* failCondition)
{
    if (!failCondition) return GameFailConditionType::None;

    if (dynamic_cast<const GameFailCondition::Score*>(failCondition)) return GameFailConditionType::Score;
    if (dynamic_cast<const GameFailCondition::Grade*>(failCondition)) return GameFailConditionType::Grade;
    if (dynamic_cast<const GameFailCondition::MissCount*>(failCondition)) return GameFailConditionType::Miss;
    if (dynamic_cast<const GameFailCondition::MissAndNearCount*>(failCondition)) return GameFailConditionType::MissAndNear;

    return GameFailConditionType::None;
}

static inline std::unique_ptr<GameFailCondition> CreateGameFailCondition(GameFailConditionType type)
{
    switch (type)
    {
    case GameFailConditionType::Score: return std::make_unique<GameFailCondition::Score>(10000000);
    case GameFailConditionType::Grade: return std::make_unique<GameFailCondition::Grade>(GradeMark::PUC);
    case GameFailConditionType::Miss: return std::make_unique<GameFailCondition::MissCount>(0);
    case GameFailConditionType::MissAndNear: return std::make_unique<GameFailCondition::MissAndNearCount>(0);
    default: return nullptr;
    }
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateFailConditionTab()
{
    Tab conditionTab = std::make_unique<TabData>();
    conditionTab->name = "Failing";

    Setting conditionType = std::make_unique<SettingData>("Fail condition", SettingType::Enum);
    for (const char* str : GAME_FAIL_CONDITION_TYPE_STR)
        conditionType->enumSetting.options.Add(str);
    conditionType->enumSetting.val = static_cast<int>(GameFailConditionType::None);
    conditionType->getter.AddLambda([this](SettingData& data) {
        data.enumSetting.val = static_cast<int>(GetGameFailConditionType(m_playOptions.failCondition.get()));
    });
    conditionType->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = CreateGameFailCondition(static_cast<GameFailConditionType>(data.enumSetting.val));
    });
    conditionTab->settings.emplace_back(std::move(conditionType));

    Setting scoreCondition = CreateIntSetting("Score less than", m_condScore, { 800 * 10000, 1000 * 10000 }, 10000);
    scoreCondition->getter.AddLambda([this](SettingData& data) {
        data.intSetting.val = m_condScore;
    });
    scoreCondition->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::Score>(data.intSetting.val);
    });
    conditionTab->settings.emplace_back(std::move(scoreCondition));

    Setting gradeCondition = std::make_unique<SettingData>("Grade less than", SettingType::Enum);
    for (const char* str : GRADE_MARK_STR)
        gradeCondition->enumSetting.options.Add(str);
    gradeCondition->enumSetting.val = static_cast<int>(m_condGrade);
    gradeCondition->getter.AddLambda([this](SettingData& data) {
        data.enumSetting.val = static_cast<int>(m_condGrade);
    });
    gradeCondition->setter.AddLambda([this](const SettingData& data) {
        m_condGrade = static_cast<GradeMark>(data.enumSetting.val);
        m_playOptions.failCondition = std::make_unique<GameFailCondition::Grade>(m_condGrade);
    });
    conditionTab->settings.emplace_back(std::move(gradeCondition));

    Setting missCondition = CreateIntSetting("Miss more than", m_condMiss, { 0, 100 });
    missCondition->getter.AddLambda([this](SettingData& data) {
        data.intSetting.val = m_condMiss;
    });
    missCondition->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::MissCount>(data.intSetting.val);
    });
    conditionTab->settings.emplace_back(std::move(missCondition));

    Setting missNearCondition = CreateIntSetting("Miss+Near more than", m_condMissNear, { 0, 100 });
    missNearCondition->getter.AddLambda([this](SettingData& data) {
        data.intSetting.val = m_condMissNear;
    });
    missNearCondition->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::MissAndNearCount>(data.intSetting.val);
    });
    conditionTab->settings.emplace_back(std::move(missNearCondition));

    return conditionTab;
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateOffsetTab()
{
    Tab offSetSettingTab = std::make_unique<TabData>();
    offSetSettingTab->name = "Offset";

    Setting globalOffsetSetting = CreateIntSetting(GameConfigKeys::GlobalOffset, "Global offset", { -200, 200 });
    globalOffsetSetting->setter.AddLambda([this](const SettingData&) { onSettingChange.Call(); });
    offSetSettingTab->settings.emplace_back(std::move(globalOffsetSetting));

    if (m_chartIndex)
    {
        Setting chartOffsetSetting = CreateIntSetting("Chart offset", m_chartIndex->custom_offset, { -200, 200 });
        chartOffsetSetting->setter.AddLambda([this](const SettingData& data) {
            onSettingChange.Call();
            });
        offSetSettingTab->settings.emplace_back(std::move(chartOffsetSetting));
    }

    Setting tempOffsetSetting = CreateIntSetting("Temporary offset", m_tempOffset, { -200, 200 });
    tempOffsetSetting->setter.AddLambda([this](const SettingData&) { onSettingChange.Call(); });
    offSetSettingTab->settings.emplace_back(std::move(tempOffsetSetting));

    return offSetSettingTab;
}
