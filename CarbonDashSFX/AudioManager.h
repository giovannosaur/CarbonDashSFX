#pragma once

#include <string>

#include "bass.h"

class AudioManager
{
public:
    bool Initialize();

    HSTREAM LoadStream(
        const std::string& path,
        bool loop
    );

    bool Play(
        HSTREAM stream,
        bool restart
    );

    bool SetVolume(
        HSTREAM stream,
        float volume
    );

    bool Stop(
        HSTREAM stream
    );

    void FreeStream(
        HSTREAM stream
    );

    void Shutdown();

private:
    bool initializedByUs_ = false;
};