#pragma once
#include "stdafx.h"
#include "cpr/cpr.h"
#include "json.hpp"
#include <Beatmap/MapDatabase.hpp>
#include <Beatmap/BeatmapObjects.hpp>

namespace IR
{
    struct ResponseState final
    {
        constexpr static int Unused = 0;
        constexpr static int Pending = 10;
        constexpr static int Success = 20;
        constexpr static int BadRequest = 40;
        constexpr static int Unauthorized = 41;
        constexpr static int ChartRefused = 42;
        constexpr static int Forbidden = 43;
        constexpr static int NotFound = 44;
        constexpr static int ServerError = 50;
        constexpr static int RequestFailure = 60;

        constexpr static std::initializer_list<std::pair<const char*, int>> Values = {
            {"Unused", IR::ResponseState::Unused},
            {"Pending", IR::ResponseState::Pending},
            {"Success", IR::ResponseState::Success},
            {"BadRequest", IR::ResponseState::BadRequest},
            {"Unauthorized", IR::ResponseState::Unauthorized},
            {"ChartRefused", IR::ResponseState::ChartRefused},
            {"Forbidden", IR::ResponseState::Forbidden},
            {"NotFound", IR::ResponseState::NotFound},
            {"ServerError", IR::ResponseState::ServerError},
            {"RequestFailure", IR::ResponseState::RequestFailure},
        };

    private:
        ResponseState() = delete;
        ~ResponseState() = delete;
    };

    cpr::AsyncResponse PostScore(const ScoreIndex& score, const BeatmapSettings& map);
    cpr::AsyncResponse Heartbeat();
    cpr::AsyncResponse ChartTracked(String chartHash);
    cpr::AsyncResponse Record(String chartHash);
    cpr::AsyncResponse Leaderboard(String chartHash, String mode, int n);
    cpr::AsyncResponse PostReplay(String identifier, String replayPath);

    bool ValidateReturn(const nlohmann::json& json);
    bool ValidatePostScoreReturn(const nlohmann::json& json);
}
