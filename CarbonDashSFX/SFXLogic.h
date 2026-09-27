#pragma once

#include "Config.h"

namespace SFXLogic
{
    bool GetTargetRPM(
        const RPMTarget& target,
        float& targetRPM
    );

    bool IsRPMTargetReached(
        const RPMTarget& target,
        float currentRPM,
        float& targetRPM
    );

    bool IsSpeedTargetReached(
        float targetSpeed,
        float currentSpeed
    );
}