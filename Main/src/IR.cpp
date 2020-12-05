#include "stdafx.h"
#include "IR.hpp"
#include "cpr/cpr.h"
#include "json.hpp"
#include "GameConfig.hpp"

void PopulateScoreJSON(nlohmann::json& json, ScoreIndex* score)
{
    json["token"] = g_gameConfig.GetString(GameConfigKeys::IRToken);

    json["score"] = score->score;
    json["crit"] = score->crit;
    json["almost"] = score->almost;
    json["miss"] = score->miss;
    json["gauge"] = score->gauge;
    json["gameflags"] = score->gameflags;
    json["timestamp"] = score->timestamp;
    json["chartHash"] = score->chartHash;

    json["hitWindowPerfect"] = score->hitWindowPerfect;
    json["hitWindowGood"] = score->hitWindowGood;
    json["hitWindowHold"] = score->hitWindowHold;
    json["hitWindowMiss"] = score->hitWindowMiss;
}

namespace IR {
    bool PostScore(ScoreIndex* score)
    {
        String host = g_gameConfig.GetString(GameConfigKeys::IRBaseURL) + "/score/submit";



        nlohmann::json json;

        PopulateScoreJSON(json, score);

        auto res = cpr::Post(cpr::Url{host},
                             cpr::Body{json.dump()});

        bool success = res.status_code == 200;

        if(!success) Logf("Submitting score to IR failed with code: %d", Logger::Severity::Warning, res.status_code);

        return success;
    }
}
