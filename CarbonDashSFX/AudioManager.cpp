#include "pch.h"

#include "AudioManager.h"
#include "Logger.h"

#include <string>


bool AudioManager::Initialize()
{
    int bassDevice = BASS_GetDevice();

    Logger::Log(
        "BASS_GetDevice() = " +
        std::to_string(bassDevice)
    );


    if (bassDevice == -1)
    {
        Logger::Log(
            "BASS is not initialized. Initializing..."
        );

        if (!BASS_Init(
            -1,
            44100,
            0,
            nullptr,
            nullptr
        ))
        {
            Logger::Log(
                "BASS_Init FAILED. Error = " +
                std::to_string(
                    BASS_ErrorGetCode()
                )
            );

            return false;
        }

        initializedByUs_ = true;

        Logger::Log(
            "BASS initialized by CarbonDashSFX."
        );
    }
    else
    {
        Logger::Log(
            "BASS already initialized. "
            "Using existing device " +
            std::to_string(bassDevice)
        );
    }

    return true;
}


HSTREAM AudioManager::LoadStream(
    const std::string& path,
    bool loop
)
{
    LOG_DEBUG(
        "Loading sound: " + path
    );

    DWORD flags = 0;

    if (loop)
        flags |= BASS_SAMPLE_LOOP;

    HSTREAM stream =
        BASS_StreamCreateFile(
            FALSE,
            path.c_str(),
            0,
            0,
            flags
        );

    if (!stream)
    {
        Logger::Log(
            "BASS_StreamCreateFile FAILED. Error = " +
            std::to_string(
                BASS_ErrorGetCode()
            )
        );

        return 0;
    }

    Logger::Log(
        "Sound loaded successfully."
    );

    return stream;
}


bool AudioManager::Play(
    HSTREAM stream,
    bool restart
)
{
    if (!stream)
        return false;

    return BASS_ChannelPlay(
        stream,
        restart
    ) != FALSE;
}

bool AudioManager::Stop(
    HSTREAM stream
)
{
    if (!stream)
        return false;

    return BASS_ChannelStop(
        stream
    ) != 0;
}

bool AudioManager::SetVolume(
    HSTREAM stream,
    float volume
)
{
    if (!stream)
        return false;

    if (volume < 0.0f)
        volume = 0.0f;

    if (volume > 1.0f)
        volume = 1.0f;

    return BASS_ChannelSetAttribute(
        stream,
        BASS_ATTRIB_VOL,
        volume
    ) != FALSE;
}

void AudioManager::FreeStream(
    HSTREAM stream
)
{
    if (!stream)
        return;

    BASS_StreamFree(stream);
}


void AudioManager::Shutdown()
{
    if (initializedByUs_)
    {
        BASS_Free();

        Logger::Log(
            "BASS freed by CarbonDashSFX."
        );

        initializedByUs_ = false;
    }
    else
    {
        Logger::Log(
            "BASS was owned by another module. "
            "CarbonDashSFX did not free it."
        );
    }
}