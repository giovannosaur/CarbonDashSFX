#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Config.h"

class AudioManager;

class SFXManager
{
public:
    SFXManager();

    bool Initialize(
        const CarbonDashConfig& config,
        AudioManager& audio
    );

    void Update();
    void Shutdown();

    size_t GetSFXCount() const;

private:
    struct Runtime
    {
        SFXConfig config;

        unsigned long stream = 0;

        std::vector<std::string> targetVehicleNames;

        bool wasAboveThreshold = false;
        bool isPlaying = false;
    };

    void StopAllStreams();

    std::vector<Runtime> runtimes_;

    AudioManager* audio_ = nullptr;

    bool lastPlaybackAllowed_ = true;
};