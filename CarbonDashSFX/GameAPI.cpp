#include "pch.h"

#include "GameAPI.h"

#include <cmath>
#include <cstdint>

namespace
{
    using DALVehicle_GetFloat_t =
        bool(__thiscall*)(void*, float*, int);


    // --------------------------------------------------------
    // RPM
    // --------------------------------------------------------

    constexpr uintptr_t ADDRESS_GET_RPM =
        0x004B65F0;

    const auto DALVehicle_GetRPM =
        reinterpret_cast<DALVehicle_GetFloat_t>(
            ADDRESS_GET_RPM
            );


    // --------------------------------------------------------
    // Redline
    // --------------------------------------------------------

    constexpr uintptr_t ADDRESS_GET_REDLINE =
        0x004B6690;

    const auto DALVehicle_GetRedline =
        reinterpret_cast<DALVehicle_GetFloat_t>(
            ADDRESS_GET_REDLINE
            );


    // --------------------------------------------------------
    // Max RPM
    // --------------------------------------------------------

    constexpr uintptr_t ADDRESS_GET_MAX_RPM =
        0x004B66E0;

    const auto DALVehicle_GetMaxRPM =
        reinterpret_cast<DALVehicle_GetFloat_t>(
            ADDRESS_GET_MAX_RPM
            );

    // Speed

    constexpr uintptr_t PLAYER_SPEED_ADDRESS =
        0x00A8E178;

    constexpr float RAW_SPEED_TO_KMH =
        3.594f;

    // --------------------------------------------------------
    // Player vehicle
    // --------------------------------------------------------

    using IPlayer_GetSimable_t =
        void* (__thiscall*)(void*);

    using IVehicle_GetVehicleName_t =
        char* (__thiscall*)(void*);

    using IVehicle_GetVehicleKey_t =
        uint32_t(__thiscall*)(void*);


    constexpr uintptr_t ADDRESS_PLAYER_LIST =
        0xA9FF5C;

    constexpr uintptr_t OFFSET_LIST_SIZE =
        0x08;

    constexpr uintptr_t IPLAYER_VTABLE_GET_SIMABLE =
        1;

    constexpr uintptr_t OFFSET_ISIMABLE_TO_PVEHICLE =
        0x24;

    constexpr uintptr_t OFFSET_PVEHICLE_TO_IVEHICLE =
        0xB8;

    constexpr uintptr_t IVehicle_VTABLE_GET_VEHICLE_NAME =
        23;

    constexpr uintptr_t IVehicle_VTABLE_GET_VEHICLE_KEY =
        24;


    bool GetPlayerVehiclePointer(
        void*& vehicle
    )
    {
        vehicle = nullptr;

        // PlayerList layout:
        //
        // +0x00 = mBegin
        // +0x04 = mCapacity
        // +0x08 = mSize
        //

        auto playerListBegin =
            *reinterpret_cast<uint8_t**>(
                ADDRESS_PLAYER_LIST
                );

        auto playerListSize =
            *reinterpret_cast<uint32_t*>(
                ADDRESS_PLAYER_LIST +
                OFFSET_LIST_SIZE
                );

        if (!playerListBegin || playerListSize == 0)
            return false;


        // PlayerList.Get(0)
        auto player =
            *reinterpret_cast<void**>(
                playerListBegin
                );

        if (!player)
            return false;


        auto playerVTable =
            *reinterpret_cast<uintptr_t**>(
                player
                );

        if (!playerVTable)
            return false;


        auto getSimable =
            reinterpret_cast<IPlayer_GetSimable_t>(
                playerVTable[
                    IPLAYER_VTABLE_GET_SIMABLE
                ]
                );

        if (!getSimable)
            return false;


        void* simable =
            getSimable(player);

        if (!simable)
            return false;


        // ISimable::GetParent() returns:
        // simable - 0x24
        //
        // PVehicle::m_sVehicle is at:
        // parent + 0xB8
        //
        // Therefore:
        //
        // IVehicle = simable + 0x94
        //

        uintptr_t vehicleAddress =
            reinterpret_cast<uintptr_t>(simable) -
            OFFSET_ISIMABLE_TO_PVEHICLE +
            OFFSET_PVEHICLE_TO_IVEHICLE;


        vehicle =
            reinterpret_cast<void*>(
                vehicleAddress
                );

        return vehicle != nullptr;
    }

    // --------------------------------------------------------
    // Gameplay state
    // --------------------------------------------------------

    constexpr uintptr_t ADDRESS_FMV_FLAG_A =
        0x00A97A80;

    constexpr uintptr_t ADDRESS_FMV_FLAG_B =
        0x00BCB020;

    constexpr uintptr_t ADDRESS_GAME_STATE =
        0xA99BBC;

    constexpr uintptr_t ADDRESS_IS_IN_NIS =
        0xB42EBC;

    constexpr uintptr_t ADDRESS_IS_GAMEPLAY_PAUSED =
        0xA8AD18;

    constexpr uintptr_t ADDRESS_WINDOW_HAS_LOST_FOCUS =
        0xAB0B3C;

    constexpr uintptr_t ADDRESS_SPEEDBREAKER =
        0x00A9922C;
}


namespace GameAPI
{
    bool GetRPM(float& rpm)
    {
        rpm = 0.0f;

        return DALVehicle_GetRPM(
            nullptr,
            &rpm,
            0
        );
    }


    bool GetRedline(float& redline)
    {
        redline = 0.0f;

        return DALVehicle_GetRedline(
            nullptr,
            &redline,
            0
        );
    }


    bool GetMaxRPM(float& maxRPM)
    {
        maxRPM = 0.0f;

        return DALVehicle_GetMaxRPM(
            nullptr,
            &maxRPM,
            0
        );
    }

    float GetPlayerSpeedKmh()
    {
        const float rawSpeed =
            *reinterpret_cast<float*>(
                PLAYER_SPEED_ADDRESS
                );

        if (!std::isfinite(rawSpeed))
            return 0.0f;

        if (rawSpeed < 0.0f)
            return 0.0f;

        return rawSpeed * RAW_SPEED_TO_KMH;
    }

    bool GetPlayerVehicleInfo(
        uint32_t& vehicleKey,
        std::string& vehicleName
    )
    {
        vehicleKey = 0;
        vehicleName.clear();


        void* vehicle = nullptr;

        if (!GetPlayerVehiclePointer(vehicle))
            return false;


        auto vehicleVTable =
            *reinterpret_cast<uintptr_t**>(
                vehicle
                );

        if (!vehicleVTable)
            return false;


        auto getVehicleName =
            reinterpret_cast<IVehicle_GetVehicleName_t>(
                vehicleVTable[
                    IVehicle_VTABLE_GET_VEHICLE_NAME
                ]
                );

        auto getVehicleKey =
            reinterpret_cast<IVehicle_GetVehicleKey_t>(
                vehicleVTable[
                    IVehicle_VTABLE_GET_VEHICLE_KEY
                ]
                );

        if (!getVehicleName || !getVehicleKey)
            return false;


        vehicleKey =
            getVehicleKey(vehicle);


        char* rawName =
            getVehicleName(vehicle);

        if (rawName)
            vehicleName = rawName;


        return true;
    }

    bool IsBlockedFMV()
    {
        const bool movieFlagA =
            (*reinterpret_cast<volatile uint8_t*>(
                ADDRESS_FMV_FLAG_A
                )) != 0;

        const bool movieFlagB =
            (*reinterpret_cast<volatile uint8_t*>(
                ADDRESS_FMV_FLAG_B
                )) != 0;

        return movieFlagA && movieFlagB;
    }


    bool IsInNIS()
    {
        return
            (*reinterpret_cast<volatile uint8_t*>(
                ADDRESS_IS_IN_NIS
                )) != 0;
    }


    bool IsGameplayPaused()
    {
        return
            (*reinterpret_cast<volatile uint8_t*>(
                ADDRESS_IS_GAMEPLAY_PAUSED
                )) != 0;
    }

    bool IsWindowFocusLost()
    {
        return
            (*reinterpret_cast<volatile uint8_t*>(
                ADDRESS_WINDOW_HAS_LOST_FOCUS
                )) != 0;
    }

    bool IsSpeedBreakerActive()
    {
        return
            (*reinterpret_cast<volatile uint8_t*>(
                ADDRESS_SPEEDBREAKER
                )) != 0;
    }


    int GetGameState()
    {
        return
            *reinterpret_cast<volatile int*>(
                ADDRESS_GAME_STATE
                );
    }


    bool IsGameplayPlaybackAllowed()
    {
        if (IsWindowFocusLost())
            return false;

        if (IsBlockedFMV())
            return false;

        if (IsInNIS())
            return false;

        if (IsGameplayPaused())
            return false;

        if (IsSpeedBreakerActive())
            return false;

        if (GetGameState() != 6)
            return false;

        return true;
    }
}