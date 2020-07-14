#include "stdafx.h"
#include "PracticeModeSettingsDialog.hpp"

PracticeModeSettingsDialog::PracticeModeSettingsDialog(MapTime endTime, MapTime& lastMapTime, Game::PlayOptions& playOptions, MapTimeRange& range)
    : m_endTime(endTime), m_lastMapTime(lastMapTime), m_playOptions(playOptions), m_range(range)
{
}

void PracticeModeSettingsDialog::InitTabs()
{
    AddTab(std::move(m_CreatePlaybackTab()));
    AddTab(std::move(m_CreateConditionTab()));

    SetCurrentTab(0);
}

const static Vector<float> SPEEDS = { 0.25f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
const static Vector<String> SPEEDS_STR = { "0.25", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0" };
static int GetSpeedInd(float f)
{
    if (f <= SPEEDS.front())
        return 0;
    if (f >= SPEEDS.back())
        return SPEEDS.size() - 1;

    int minInd = 0;
    for (int i = 1; i < SPEEDS.size(); ++i)
    {
        if (std::abs(f - SPEEDS[i]) < std::abs(f - SPEEDS[minInd]))
            minInd = i;
    }

    return minInd;
}
static float GetSpeed(const String& str)
{
    return SPEEDS[std::distance(SPEEDS_STR.begin(), std::find(SPEEDS_STR.begin(), SPEEDS_STR.end(), str))];
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreatePlaybackTab()
{
    Tab playbackTab = std::make_unique<TabData>();
    playbackTab->name = "Playback";

    Setting loopBeginButton = std::make_unique<SettingData>();
    loopBeginButton->name = "Set start point to here";
    loopBeginButton->type = SettingType::Button;
    loopBeginButton->boolSetting.val = false;
    loopBeginButton->boolSetting.setter.AddLambda([this](bool value) { m_range.begin = Math::Clamp(m_lastMapTime, 0, m_endTime); });
    playbackTab->settings.emplace_back(std::move(loopBeginButton));

    Setting loopEndButton = std::make_unique<SettingData>();
    loopEndButton->name = "Set end point to here";
    loopEndButton->type = SettingType::Button;
    loopEndButton->boolSetting.val = false;
    loopEndButton->boolSetting.setter.AddLambda([this](bool value) { m_range.end = Math::Clamp(m_lastMapTime, 0, m_endTime); if (!m_range.HasEnd()) m_range.end = 0; });
    playbackTab->settings.emplace_back(std::move(loopEndButton));

    Setting loopBeginSetting = std::make_unique<SettingData>();
    loopBeginSetting->name = "Start point";
    loopBeginSetting->type = SettingType::Integer;
    loopBeginSetting->intSetting.val = 0;
    loopBeginSetting->intSetting.min = 0;
    loopBeginSetting->intSetting.max = m_endTime;
    loopBeginSetting->intSetting.step = 50;
    loopBeginSetting->intSetting.setter.AddLambda([this](int value) { m_range.begin = value; onSetMapTime.Call(value); });
    loopBeginSetting->intSetting.getter.AddLambda([this](int& value) { value = m_range.begin; });
    playbackTab->settings.emplace_back(std::move(loopBeginSetting));

    Setting loopEndSetting = std::make_unique<SettingData>();
    loopEndSetting->name = "End point";
    loopEndSetting->type = SettingType::Integer;
    loopEndSetting->intSetting.val = 0;
    loopEndSetting->intSetting.min = 0;
    loopEndSetting->intSetting.max = m_endTime;
    loopEndSetting->intSetting.step = 50;
    loopEndSetting->intSetting.setter.AddLambda([this](int value) { m_range.end = value; onSetMapTime.Call(value); });
    loopEndSetting->intSetting.getter.AddLambda([this](int& value) { value = m_range.end; });
    playbackTab->settings.emplace_back(std::move(loopEndSetting));

    Setting loopOnSuccess = std::make_unique<SettingData>();
    loopOnSuccess->name = "Loop on success";
    loopOnSuccess->type = SettingType::Toggle;
    loopOnSuccess->boolSetting.val = m_playOptions.loopOnSuccess;
    loopOnSuccess->boolSetting.setter.AddLambda([this](bool value) { m_playOptions.loopOnSuccess = value; });
    loopOnSuccess->boolSetting.getter.AddLambda([this](bool& value) { value = m_playOptions.loopOnSuccess; });
    playbackTab->settings.emplace_back(std::move(loopOnSuccess));

    Setting loopOnFail = std::make_unique<SettingData>();
    loopOnFail->name = "Loop on fail";
    loopOnFail->type = SettingType::Toggle;
    loopOnFail->boolSetting.val = m_playOptions.loopOnFail;
    loopOnFail->boolSetting.setter.AddLambda([this](bool value) { m_playOptions.loopOnFail = value; });
    loopOnFail->boolSetting.getter.AddLambda([this](bool& value) { value = m_playOptions.loopOnFail; });
    playbackTab->settings.emplace_back(std::move(loopOnFail));

    assert(SPEEDS.size() == SPEEDS_STR.size());

    Setting speedSetting = std::make_unique<SettingData>();
    speedSetting->name = "Playback speed";
    speedSetting->type = SettingType::Enum;
    speedSetting->enumSetting.options = SPEEDS_STR;
    speedSetting->enumSetting.val = GetSpeedInd(m_playOptions.playbackSpeed);
    speedSetting->enumSetting.setter.AddLambda([this](String value) { onSpeedChange.Call(GetSpeed(value)); });
    speedSetting->enumSetting.getter.AddLambda([this](String& value) { value = SPEEDS_STR[GetSpeedInd(m_playOptions.playbackSpeed)]; });
    playbackTab->settings.emplace_back(std::move(speedSetting));

    return playbackTab;
}

PracticeModeSettingsDialog::Tab PracticeModeSettingsDialog::m_CreateConditionTab()
{
    Tab conditionTab = std::make_unique<TabData>();
    conditionTab->name = "Condition";

    Setting conditionType = std::make_unique<SettingData>();
    conditionType->name = "Fail condition";
    conditionType->type = SettingType::Enum;
    conditionType->enumSetting.options = { "None", "Score", "Grade", "Miss", "Miss+Near" };
    conditionType->enumSetting.val = 0;
    conditionType->enumSetting.getter.AddLambda([this](String& value) {
        auto* failCondition = m_playOptions.failCondition.get();
        if (dynamic_cast<GameFailCondition::Score*>(failCondition)) { value = "Score"; return; }
        if (dynamic_cast<GameFailCondition::Grade*>(failCondition)) { value = "Grade"; return; }
        if (dynamic_cast<GameFailCondition::MissCount*>(failCondition)) { value = "Miss"; return; }
        if (dynamic_cast<GameFailCondition::MissAndNearCount*>(failCondition)) { value = "Miss+Near"; return; }

        m_playOptions.failCondition = nullptr;
        value = "None";
    });
    conditionType->enumSetting.setter.AddLambda([this](String value) {
        if (value == "None") m_playOptions.failCondition = nullptr;
        if (value == "Score") m_playOptions.failCondition = std::make_unique<GameFailCondition::Score>(10000000);
        if (value == "Grade") m_playOptions.failCondition = std::make_unique<GameFailCondition::Grade>(GradeMark::PUC);
        if (value == "Miss") m_playOptions.failCondition = std::make_unique<GameFailCondition::MissCount>(0);
        if (value == "Miss+Near") m_playOptions.failCondition = std::make_unique<GameFailCondition::MissAndNearCount>(0);
    });
    conditionTab->settings.emplace_back(std::move(conditionType));

    Setting scoreThreshold = std::make_unique<SettingData>();
    scoreThreshold->name = "Score less than";
    scoreThreshold->type = SettingType::Integer;
    scoreThreshold->intSetting.min = 800 * 10000;
    scoreThreshold->intSetting.max = 1000 * 10000;
    scoreThreshold->intSetting.step = 10000;
    scoreThreshold->intSetting.val = 10000000;
    scoreThreshold->intSetting.getter.AddLambda([this](int& value) {
        if (auto* cond = dynamic_cast<GameFailCondition::Score*>(m_playOptions.failCondition.get()))
            value = cond->GetMinAllowed();
    });
    scoreThreshold->intSetting.setter.AddLambda([this](int value) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::Score>(value);
    });
    conditionTab->settings.emplace_back(std::move(scoreThreshold));

    Setting gradeThreshold = std::make_unique<SettingData>();
    gradeThreshold->name = "Grade less than";
    gradeThreshold->type = SettingType::Enum;
    gradeThreshold->enumSetting.options = { "A", "A+", "AA", "AA+", "AAA", "AAA+", "S", "995", "PUC" };
    gradeThreshold->enumSetting.val = 0;
    gradeThreshold->enumSetting.getter.AddLambda([this](String& value) {
        value = "PUC";
        if (auto* cond = dynamic_cast<GameFailCondition::Grade*>(m_playOptions.failCondition.get()))
        {
            switch (cond->GetMinAllowed())
            {
            case GradeMark::A: value = "A"; break;
            case GradeMark::Ap: value = "A+"; break;
            case GradeMark::AA: value = "AA"; break;
            case GradeMark::AAp: value = "AA+"; break;
            case GradeMark::AAA: value = "AAA"; break;
            case GradeMark::AAAp: value = "AAA+"; break;
            case GradeMark::S: value = "S"; break;
            case GradeMark::S_995: value = "995"; break;
            case GradeMark::PUC: value = "PUC"; break;
            }
        }
    });
    gradeThreshold->enumSetting.setter.AddLambda([this](String value) {
        GradeMark mark = GradeMark::D;
        if (value == "A") mark = GradeMark::A;
        if (value == "A+") mark = GradeMark::Ap;
        if (value == "AA") mark = GradeMark::AA;
        if (value == "AA+") mark = GradeMark::AAp;
        if (value == "AAA") mark = GradeMark::AAA;
        if (value == "AAA+") mark = GradeMark::AAAp;
        if (value == "S") mark = GradeMark::S;
        if (value == "995") mark = GradeMark::S_995;
        if (value == "PUC") mark = GradeMark::PUC;
        m_playOptions.failCondition = std::make_unique<GameFailCondition::Grade>(mark);
    });
    conditionTab->settings.emplace_back(std::move(gradeThreshold));

    Setting missThreshold = std::make_unique<SettingData>();
    missThreshold->name = "Miss count more than";
    missThreshold->type = SettingType::Integer;
    missThreshold->intSetting.min = 0;
    missThreshold->intSetting.max = 100;
    missThreshold->intSetting.val = 0;
    missThreshold->intSetting.getter.AddLambda([this](int& value) {
        if (auto* cond = dynamic_cast<GameFailCondition::MissCount*>(m_playOptions.failCondition.get()))
            value = cond->GetMaxAllowed();
    });
    missThreshold->intSetting.setter.AddLambda([this](int value) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::MissCount>(value);
    });
    conditionTab->settings.emplace_back(std::move(missThreshold));

    Setting missNearThreshold = std::make_unique<SettingData>();
    missNearThreshold->name = "Miss+Near count more than";
    missNearThreshold->type = SettingType::Integer;
    missNearThreshold->intSetting.min = 0;
    missNearThreshold->intSetting.max = 100;
    missNearThreshold->intSetting.val = 0;
    missNearThreshold->intSetting.getter.AddLambda([this](int& value) {
        if (auto* cond = dynamic_cast<GameFailCondition::MissAndNearCount*>(m_playOptions.failCondition.get()))
            value = cond->GetMaxAllowed();
        });
    missNearThreshold->intSetting.setter.AddLambda([this](int value) {
        m_playOptions.failCondition = std::make_unique<GameFailCondition::MissAndNearCount>(value);
    });
    conditionTab->settings.emplace_back(std::move(missNearThreshold));

    return conditionTab;
}
