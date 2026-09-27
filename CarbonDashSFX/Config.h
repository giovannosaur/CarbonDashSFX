#pragma once

#include <string>
#include <vector>


enum class RPMTargetType
{
    None,
    Fixed,
    Redline,
    MaxRPM
};


struct RPMTarget
{
    RPMTargetType type =
        RPMTargetType::None;

    float modifier = 0.0f;

    float fixedValue = 0.0f;
};


struct SFXConfig
{
    int type = 0;

    int volume = 100;

    RPMTarget rpmTarget;

    float targetSpeed = 0.0f;

    std::string file;

    std::vector<std::string> targetVehicles;
};


struct CarbonDashConfig
{
    bool enabled = true;

    std::vector<SFXConfig> sfx;
};


namespace Config
{
    bool Load(
        const std::string& path,
        CarbonDashConfig& config
    );
}