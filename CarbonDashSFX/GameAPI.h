#pragma once

#include <cstdint>
#include <string>

namespace GameAPI
{
    bool GetRPM(float& rpm);
    bool GetRedline(float& redline);
    bool GetMaxRPM(float& maxRPM);

    float GetPlayerSpeedKmh();

    bool GetPlayerVehicleInfo(
        uint32_t& vehicleKey,
        std::string& vehicleName
    );

    bool IsBlockedFMV();
    bool IsInNIS();
    bool IsGameplayPaused();
    bool IsWindowFocusLost();
    bool IsSpeedBreakerActive();
    int GetGameState();

    bool IsGameplayPlaybackAllowed();
}