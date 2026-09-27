#include "pch.h"

#include "SFXManager.h"
#include "AudioManager.h"
#include "Logger.h"
#include "Paths.h"
#include "GameAPI.h"
#include "SFXLogic.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
    std::string NormalizeVehicleName(
        std::string value
    )
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(
                    std::tolower(c)
                    );
            }
        );

        return value;
    }
}

SFXManager::SFXManager()
{
}


bool SFXManager::Initialize(
    const CarbonDashConfig& config,
    AudioManager& audio
)
{
    audio_ = &audio;

    runtimes_.clear();

    runtimes_.reserve(
        config.sfx.size()
    );


    Logger::Log(
        "SFXManager: initializing " +
        std::to_string(
            config.sfx.size()
        ) +
        " SFX entries."
    );


    for (size_t i = 0;
        i < config.sfx.size();
        ++i)
    {
        const SFXConfig& sfx =
            config.sfx[i];


        Runtime runtime;

        runtime.config =
            sfx;

        runtime.stream = 0;

        runtime.wasAboveThreshold =
            false;

        for (const std::string& vehicleName :
            sfx.targetVehicles)
        {
            std::string normalized =
                NormalizeVehicleName(
                    vehicleName
                );

            if (normalized.empty())
                continue;

            runtime.targetVehicleNames.push_back(
                normalized
            );

            Logger::Log(
                "SFXManager: SFX[" +
                std::to_string(i + 1) +
                "] target vehicle: " +
                normalized
            );
        }


        Logger::Log(
            "SFXManager: initializing SFX[" +
            std::to_string(i + 1) +
            "]"
        );


        // ----------------------------------------------------
        // SFX Types
        // ----------------------------------------------------

        if (
            sfx.type != 0 &&
            sfx.type != 1 &&
            sfx.type != 2
            )
        {
            Logger::Log(
                "SFXManager: SFX[" +
                std::to_string(i + 1) +
                "] skipped. "
                "Unsupported Type."
            );

            runtimes_.push_back(runtime);
            continue;
        }


        // ----------------------------------------------------
        // Load audio file.
        // ----------------------------------------------------

        const std::string path =
            Paths::SFXDirectory() +
            "\\" +
            sfx.file;


        runtime.stream =
            audio.LoadStream(
                path,
                sfx.type == 0 ||
                sfx.type == 2
            );


        if (!runtime.stream)
        {
            Logger::Log(
                "SFXManager: SFX[" +
                std::to_string(i + 1) +
                "] failed to load audio."
            );

            runtimes_.push_back(
                runtime
            );

            continue;
        }


        const float volume =
            static_cast<float>(
                sfx.volume
                ) / 100.0f;


        if (!audio.SetVolume(
            runtime.stream,
            volume
        ))
        {
            Logger::Log(
                "SFXManager: SFX[" +
                std::to_string(i + 1) +
                "] failed to set volume."
            );

            audio.FreeStream(
                runtime.stream
            );

            runtime.stream = 0;

            runtimes_.push_back(
                runtime
            );

            continue;
        }


        Logger::Log(
            "SFXManager: SFX[" +
            std::to_string(i + 1) +
            "] loaded successfully. "
            "Volume = " +
            std::to_string(
                sfx.volume
            )
        );

        runtimes_.push_back(
            runtime
        );
    }


    Logger::Log(
        "SFXManager: initialization complete."
    );


    return true;
}

void SFXManager::StopAllStreams()
{
    if (!audio_)
        return;

    for (auto& runtime : runtimes_)
    {
        if (runtime.stream != 0)
        {
            audio_->Stop(
                runtime.stream
            );

            runtime.isPlaying = false;
        }
    }
}

void SFXManager::Update()
{
    if (!audio_)
        return;


    bool playbackAllowed =
        GameAPI::IsGameplayPlaybackAllowed();

    bool playbackStateChanged =
        playbackAllowed != lastPlaybackAllowed_;

    bool enteredAllowedState =
        playbackAllowed && !lastPlaybackAllowed_;


    if (playbackStateChanged)
    {
        LOG_DEBUG(
            "SFXManager: playback state changed: " +
            std::string(
                playbackAllowed
                ? "ALLOWED"
                : "BLOCKED"
            )
        );

        LOG_DEBUG(
            "  GameState = " +
            std::to_string(
                GameAPI::GetGameState()
            )
        );

        LOG_DEBUG(
            "  FMV = " +
            std::to_string(
                GameAPI::IsBlockedFMV()
                ? 1
                : 0
            )
        );

        LOG_DEBUG(
            "  NIS = " +
            std::to_string(
                GameAPI::IsInNIS()
                ? 1
                : 0
            )
        );

        LOG_DEBUG(
            "  Paused = " +
            std::to_string(
                GameAPI::IsGameplayPaused()
                ? 1
                : 0
            )
        );

        LOG_DEBUG(
            "  FocusLost = " +
            std::to_string(
                GameAPI::IsWindowFocusLost()
                ? 1
                : 0
            )
        );

        LOG_DEBUG(
            "  SpeedBreaker = " +
            std::to_string(
                GameAPI::IsSpeedBreakerActive()
                ? 1
                : 0
            )
        );


        if (!playbackAllowed)
        {
            StopAllStreams();
        }


        lastPlaybackAllowed_ =
            playbackAllowed;
    }


    if (!playbackAllowed)
    {
        return;
    }

    uint32_t currentVehicleKey = 0;
    std::string currentVehicleName;

    if (!GameAPI::GetPlayerVehicleInfo(
        currentVehicleKey,
        currentVehicleName
    ))
    {
        StopAllStreams();

        for (auto& runtime : runtimes_)
        {
            runtime.wasAboveThreshold = false;
            runtime.isPlaying = false;
        }

        return;
    }

    std::string normalizedCurrentVehicleName =
        NormalizeVehicleName(
            currentVehicleName
        );

    float speedKmh =
        GameAPI::GetPlayerSpeedKmh();

    float rpm = 0.0f;

    bool rpmRequired = false;

    for (const auto& runtime : runtimes_)
    {
        if (
            runtime.stream != 0 &&
            (
                runtime.config.type == 1 ||
                runtime.config.type == 2
                )
            )
        {
            rpmRequired = true;
            break;
        }
    }

    if (
        rpmRequired &&
        !GameAPI::GetRPM(rpm)
        )
    {
        StopAllStreams();

        for (auto& runtime : runtimes_)
        {
            runtime.wasAboveThreshold = false;
        }

        return;
    }

    // --------------------------------------------------------
    // Evaluate every SFX entry independently.
    // --------------------------------------------------------

    for (size_t i = 0;
        i < runtimes_.size();
        ++i)
    {
        Runtime& runtime =
            runtimes_[i];


        if (runtime.stream == 0)
            continue;

        bool vehicleMatch = true;

        if (!runtime.targetVehicleNames.empty())
        {
            vehicleMatch = false;

            for (const std::string& targetName :
                runtime.targetVehicleNames)
            {
                if (targetName ==
                    normalizedCurrentVehicleName)
                {
                    vehicleMatch = true;
                    break;
                }
            }
        }

        if (!vehicleMatch)
        {
            runtime.wasAboveThreshold = false;

            if (
                (
                    runtime.config.type == 0 ||
                    runtime.config.type == 2
                    ) &&
                runtime.isPlaying
                )
            {
                audio_->Stop(
                    runtime.stream
                );

                runtime.isPlaying = false;
            }

            continue;
        }

        if (runtime.config.type == 0)
        {
            // ----------------------------------------------------
            // Type 0: loop while speed is at or above threshold.
            // Returning from BLOCKED resumes immediately if the
            // current speed is still above the target.
            // ----------------------------------------------------

            bool speedReached =
                SFXLogic::IsSpeedTargetReached(
                    runtime.config.targetSpeed,
                    speedKmh
                );

            if (speedReached)
            {
                if (!runtime.isPlaying)
                {
                    LOG_DEBUG(
                        "SFXManager: SFX[" +
                        std::to_string(i + 1) +
                        "] speed loop started: " +
                        std::to_string(speedKmh) +
                        " / " +
                        std::to_string(
                            runtime.config.targetSpeed
                        ) +
                        " km/h"
                    );

                    if (audio_->Play(
                        runtime.stream,
                        true
                    ))
                    {
                        runtime.isPlaying = true;
                    }
                }
            }
            else
            {
                if (runtime.isPlaying)
                {
                    LOG_DEBUG(
                        "SFXManager: SFX[" +
                        std::to_string(i + 1) +
                        "] speed loop stopped: " +
                        std::to_string(speedKmh) +
                        " / " +
                        std::to_string(
                            runtime.config.targetSpeed
                        ) +
                        " km/h"
                    );

                    audio_->Stop(
                        runtime.stream
                    );

                    runtime.isPlaying = false;
                }
            }

            continue;
        }


        if (runtime.config.type == 1)
        {
            // ----------------------------------------------------
            // Type 1: play once when crossing the RPM threshold.
            // Returning from BLOCKED must not create a new trigger.
            // ----------------------------------------------------

            float targetRPM = 0.0f;

            bool targetReached =
                SFXLogic::IsRPMTargetReached(
                    runtime.config.rpmTarget,
                    rpm,
                    targetRPM
                );

            if (enteredAllowedState)
            {
                runtime.wasAboveThreshold =
                    targetReached;

                continue;
            }

            if (
                targetReached &&
                !runtime.wasAboveThreshold
                )
            {
                LOG_DEBUG(
                    "SFXManager: SFX[" +
                    std::to_string(i + 1) +
                    "] threshold reached: " +
                    std::to_string(rpm) +
                    " / " +
                    std::to_string(targetRPM)
                );

                if (audio_->Play(
                    runtime.stream,
                    true
                ))
                {
                    runtime.isPlaying = true;
                }
            }

            runtime.wasAboveThreshold =
                targetReached;

            continue;
        }


        if (runtime.config.type == 2)
        {
            // ----------------------------------------------------
            // Type 2: loop while RPM is at or above threshold.
            // ----------------------------------------------------

            float targetRPM = 0.0f;

            bool targetReached =
                SFXLogic::IsRPMTargetReached(
                    runtime.config.rpmTarget,
                    rpm,
                    targetRPM
                );

            if (targetReached)
            {
                if (!runtime.isPlaying)
                {
                    LOG_DEBUG(
                        "SFXManager: SFX[" +
                        std::to_string(i + 1) +
                        "] loop started: " +
                        std::to_string(rpm) +
                        " / " +
                        std::to_string(targetRPM)
                    );

                    if (audio_->Play(
                        runtime.stream,
                        true
                    ))
                    {
                        runtime.isPlaying = true;
                    }
                }
            }
            else
            {
                if (runtime.isPlaying)
                {
                    LOG_DEBUG(
                        "SFXManager: SFX[" +
                        std::to_string(i + 1) +
                        "] loop stopped: " +
                        std::to_string(rpm) +
                        " / " +
                        std::to_string(targetRPM)
                    );

                    audio_->Stop(
                        runtime.stream
                    );

                    runtime.isPlaying = false;
                }
            }
        }
    }
}


void SFXManager::Shutdown()
{
    if (!audio_)
        return;


    for (auto& runtime : runtimes_)
    {
        if (runtime.stream != 0)
        {
            audio_->FreeStream(
                runtime.stream
            );

            runtime.stream = 0;
        }
    }


    runtimes_.clear();

    audio_ = nullptr;


    Logger::Log(
        "SFXManager: shutdown complete."
    );
}


size_t SFXManager::GetSFXCount() const
{
    return runtimes_.size();
}