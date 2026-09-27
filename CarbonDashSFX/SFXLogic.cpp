#include "pch.h"

#include "SFXLogic.h"
#include "GameAPI.h"


namespace SFXLogic
{
    bool GetTargetRPM(
        const RPMTarget& target,
        float& targetRPM
    )
    {
        targetRPM = 0.0f;


        switch (target.type)
        {
        case RPMTargetType::Fixed:
        {
            targetRPM =
                target.fixedValue;

            return true;
        }


        case RPMTargetType::Redline:
        {
            float redline = 0.0f;

            if (!GameAPI::GetRedline(redline))
                return false;

            targetRPM =
                redline +
                target.modifier;

            return true;
        }


        case RPMTargetType::MaxRPM:
        {
            float maxRPM = 0.0f;

            if (!GameAPI::GetMaxRPM(maxRPM))
                return false;

            targetRPM =
                maxRPM +
                target.modifier;

            return true;
        }


        case RPMTargetType::None:
        default:
            return false;
        }
    }


    bool IsRPMTargetReached(
        const RPMTarget& target,
        float currentRPM,
        float& targetRPM
    )
    {
        if (!GetTargetRPM(
            target,
            targetRPM
        ))
        {
            return false;
        }


        return currentRPM >= targetRPM;
    }

    bool IsSpeedTargetReached(
        float targetSpeed,
        float currentSpeed
    )
    {
        return currentSpeed >= targetSpeed;
    }
}