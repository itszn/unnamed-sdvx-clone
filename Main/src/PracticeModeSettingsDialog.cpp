#include "stdafx.h"
#include "PracticeModeSettingsDialog.hpp"

PracticeModeSettingsDialog::PracticeModeSettingsDialog(Ref<Beatmap> beatmap, MapTime endTime, MapTime& lastMapTime, Game::PlayOptions& playOptions, MapTimeRange& range)
    : m_beatmap(beatmap), m_endTime(endTime), m_lastMapTime(lastMapTime), m_playOptions(playOptions), m_range(range)
{
}

void PracticeModeSettingsDialog::InitTabs()
{
    AddTab(std::move(m_CreatePlaybackTab()));
    AddTab(std::move(m_CreateFailConditionTab()));
    AddTab(std::move(m_CreateGameSettingTab()));

    SetCurrentTab(0);
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreatePlaybackTab()
{
    Tab playbackTab = std::make_unique<TabData>();
    playbackTab->name = "Playback";

    // Loop begin
    {
        Setting loopBeginMeasureSetting = CreateIntSetting("Start point (measure #)", m_startMeasure, {1, m_TimeToMeasure(m_endTime)});
        loopBeginMeasureSetting->setter.AddLambda([this](const SettingData& data) {
            m_range.begin = m_MeasureToTime(data.intSetting.val-1);
            onSetMapTime.Call(m_range.begin);
            if (m_range.end < m_range.begin)
            {
                m_range.end = m_range.begin;
                m_endMeasure = data.intSetting.val;
            }
        });
        playbackTab->settings.emplace_back(std::move(loopBeginMeasureSetting));

        Setting loopBeginMSSetting = CreateIntSetting("- in milliseconds", m_range.begin, {0, m_endTime}, 50);
        loopBeginMSSetting->setter.AddLambda([this](const SettingData& data) {
            m_startMeasure = m_TimeToMeasure(data.intSetting.val)+1;
            onSetMapTime.Call(data.intSetting.val);
            if (m_range.end < data.intSetting.val)
            {
                m_range.end = data.intSetting.val;
                m_endMeasure = m_startMeasure;
            }
        });
        playbackTab->settings.emplace_back(std::move(loopBeginMSSetting));

        Setting loopBeginButton = CreateButton("Set to here",
            std::move([this](const auto&) {
                m_range.begin = Math::Clamp(m_lastMapTime, 0, m_endTime);
                m_startMeasure = m_TimeToMeasure(m_range.begin) + 1;
                if (m_range.end < m_range.begin)
                {
                    m_range.end = m_range.begin;
                    m_endMeasure = m_startMeasure;
                }
            }
        ));
        playbackTab->settings.emplace_back(std::move(loopBeginButton));
    }

    // Loop end
    {
        Setting loopEndMeasureSetting = CreateIntSetting("End point (measure #)", m_endMeasure, {1, m_TimeToMeasure(m_endTime)});
        loopEndMeasureSetting->setter.AddLambda([this](const SettingData& data) {
            m_range.end = m_MeasureToTime(data.intSetting.val-1);
            onSetMapTime.Call(m_range.end);
        });
        playbackTab->settings.emplace_back(std::move(loopEndMeasureSetting));

        Setting loopEndMSSetting = CreateIntSetting("- in milliseconds", m_range.end, {0, m_endTime}, 50);
        loopEndMSSetting->setter.AddLambda([this](const SettingData& data) {
            m_endMeasure = m_TimeToMeasure(data.intSetting.val)+1;
            onSetMapTime.Call(data.intSetting.val);
        });
        playbackTab->settings.emplace_back(std::move(loopEndMSSetting));

        Setting loopEndClearButton = CreateButton("Clear",
            std::move([this](const auto&) { m_range.end = 0; }));
        playbackTab->settings.emplace_back(std::move(loopEndClearButton));

        Setting loopEndButton = CreateButton("Set to here",
            std::move([this](const auto&) {
                m_range.end = Math::Clamp(m_lastMapTime, 0, m_endTime);
                m_endMeasure = m_TimeToMeasure(m_range.end) + 1;
            }
        ));
        playbackTab->settings.emplace_back(std::move(loopEndButton));
    }

    Setting loopOnSuccess = CreateBoolSetting("Loop on success", m_playOptions.loopOnSuccess);
    playbackTab->settings.emplace_back(std::move(loopOnSuccess));

    Setting loopOnFail = CreateBoolSetting("Loop on fail", m_playOptions.loopOnFail);
    playbackTab->settings.emplace_back(std::move(loopOnFail));

    Setting speedSetting = std::make_unique<SettingData>("Playback speed (%)", SettingType::Integer);
    speedSetting->intSetting.min = 25;
    speedSetting->intSetting.max = 100;
    speedSetting->intSetting.val = Math::Round(m_playOptions.playbackSpeed * 100);
    speedSetting->setter.AddLambda([this](const SettingData& data) { onSpeedChange.Call(data.intSetting.val == 100 ? 1.0f : data.intSetting.val / 100.0f); });
    speedSetting->getter.AddLambda([this](SettingData& data) { data.enumSetting.val = Math::Round(m_playOptions.playbackSpeed * 100); });
    playbackTab->settings.emplace_back(std::move(speedSetting));

    return playbackTab;
}

enum class GameFailConditionType
{
    None, Score, Grade, Miss, MissAndNear
};

const char* GAME_FAIL_CONDITION_TYPE_STR[5] = { "None", "Score", "Grade", "Miss", "Miss+Near" };

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
        data.intSetting.val = static_cast<int>(GetGameFailConditionType(m_playOptions.failCondition.get()));
    });
    conditionType->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = CreateGameFailCondition(static_cast<GameFailConditionType>(data.enumSetting.val));
    });
    conditionTab->settings.emplace_back(std::move(conditionType));

    Setting scoreThreshold = std::make_unique<SettingData>("Score less than", SettingType::Integer);
    scoreThreshold->intSetting.min = 800 * 10000;
    scoreThreshold->intSetting.max = 1000 * 10000;
    scoreThreshold->intSetting.step = 10000;
    scoreThreshold->intSetting.val = 10000000;
    scoreThreshold->getter.AddLambda([this](SettingData& data) {
        if (auto* cond = dynamic_cast<GameFailCondition::Score*>(m_playOptions.failCondition.get()))
            data.intSetting.val = cond->GetMinAllowed();
    });
    scoreThreshold->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::Score>(data.intSetting.val);
    });
    conditionTab->settings.emplace_back(std::move(scoreThreshold));

    Setting gradeThreshold = std::make_unique<SettingData>("Grade less than", SettingType::Enum);
    for (const char* str : GRADE_MARK_STR)
        gradeThreshold->enumSetting.options.Add(str);
    gradeThreshold->enumSetting.val = static_cast<int>(GradeMark::PUC);
    gradeThreshold->getter.AddLambda([this](SettingData& data) {
        if (auto* cond = dynamic_cast<GameFailCondition::Grade*>(m_playOptions.failCondition.get()))
        {
            data.enumSetting.val = static_cast<int>(cond->GetMinAllowed());
        }
    });
    gradeThreshold->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::Grade>(static_cast<GradeMark>(data.enumSetting.val));
    });
    conditionTab->settings.emplace_back(std::move(gradeThreshold));

    Setting missThreshold = std::make_unique<SettingData>("Miss count more than", SettingType::Integer);
    missThreshold->intSetting.min = 0;
    missThreshold->intSetting.max = 100;
    missThreshold->intSetting.val = 0;
    missThreshold->getter.AddLambda([this](SettingData& data) {
        if (auto* cond = dynamic_cast<GameFailCondition::MissCount*>(m_playOptions.failCondition.get()))
            data.intSetting.val = cond->GetMaxAllowed();
    });
    missThreshold->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::MissCount>(data.intSetting.val);
    });
    conditionTab->settings.emplace_back(std::move(missThreshold));

    Setting missNearThreshold = std::make_unique<SettingData>("Miss+Near count more than", SettingType::Integer);
    missNearThreshold->intSetting.min = 0;
    missNearThreshold->intSetting.max = 100;
    missNearThreshold->intSetting.val = 0;
    missNearThreshold->getter.AddLambda([this](SettingData& data) {
        if (auto* cond = dynamic_cast<GameFailCondition::MissAndNearCount*>(m_playOptions.failCondition.get()))
            data.intSetting.val = cond->GetMaxAllowed();
    });
    missNearThreshold->setter.AddLambda([this](const SettingData& data) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::MissAndNearCount>(data.intSetting.val);
    });
    conditionTab->settings.emplace_back(std::move(missNearThreshold));

    return conditionTab;
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateGameSettingTab()
{
    Tab gameSettingTab = std::make_unique<TabData>();
    gameSettingTab->name = "Game";

    Setting globalOffsetSetting = CreateIntSetting(GameConfigKeys::GlobalOffset, "Global offset", { -200, 200 });
    globalOffsetSetting->setter.AddLambda([this](const SettingData&) { onSettingChange.Call(); });
    gameSettingTab->settings.emplace_back(std::move(globalOffsetSetting));

    Setting chartOffsetSetting = std::make_unique<SettingData>("Chart offset", SettingType::Integer);
    chartOffsetSetting->intSetting.min = -200;
    chartOffsetSetting->intSetting.max = 200;
    chartOffsetSetting->intSetting.val = 0;
    // TODO: get and set the chart offset
    gameSettingTab->settings.emplace_back(std::move(chartOffsetSetting));

    // TODO: speed mod
    // TODO: hidden and sudden

    return gameSettingTab;
}

constexpr double MEASURE_EPSILON = 0.0001;

inline int GetBarCount(const TimingPoint* a, const TimingPoint* b)
{
    const MapTime measureDuration = b->time - a->time;
    const double barCount = measureDuration / a->GetBarDuration();
    int barCountInt = Math::Round(barCount);

    if (std::abs(barCount - static_cast<double>(barCountInt)) >= MEASURE_EPSILON)
    {
        Logf("A timing point at %d contains non-integer # of bars: %g", Logger::Severity::Warning, a->time, barCount);
        if (barCount > barCountInt) ++barCountInt;
    }

    return barCountInt;
}

MapTime PracticeModeSettingsDialog::m_MeasureToTime(int measure)
{
    if (measure < 0) return 0;

    int currMeasureCount = 0;

    auto& timingPoints = m_beatmap->GetLinearTimingPoints();
    for (int i = 0; i < timingPoints.size(); ++i)
    {
        bool isInCurrentTimingPoint = false;
        if (i == timingPoints.size() - 1 || measure <= currMeasureCount)
        {
            isInCurrentTimingPoint = true;
        }
        else
        {
            const int barCount = GetBarCount(timingPoints[i], timingPoints[i+1]);

            if (measure < currMeasureCount + barCount)
                isInCurrentTimingPoint = true;
            else
                currMeasureCount += barCount;
        }
        if (isInCurrentTimingPoint)
        {
            measure -= currMeasureCount;
            return static_cast<MapTime>(timingPoints[i]->time + timingPoints[i]->GetBarDuration() * measure);
        }
    }

    assert(false);
    return 0;
}

int PracticeModeSettingsDialog::m_TimeToMeasure(MapTime time)
{
    if (time <= 0) return 0;

    int currMeasureCount = 0;

    auto& timingPoints = m_beatmap->GetLinearTimingPoints();

    for (int i = 0; i < timingPoints.size(); ++i)
    {
        if (i < timingPoints.size() - 1 && timingPoints[i + 1]->time <= time)
        {
            currMeasureCount += GetBarCount(timingPoints[i], timingPoints[i + 1]);
            continue;
        }

        return currMeasureCount + static_cast<int>((time - timingPoints[i]->time) / timingPoints[i]->GetBarDuration());
    }

    assert(false);
    return 0;
}
