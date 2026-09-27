#include "pch.h"

#include <windows.h>

#include "AudioManager.h"
#include "Config.h"
#include "Logger.h"
#include "Paths.h"
#include "SFXManager.h"

static constexpr DWORD POLL_INTERVAL_MS = 20;

static HANDLE g_stopEvent = nullptr;
static HANDLE g_workerThread = nullptr;

// ============================================================
// Worker thread
// ============================================================

DWORD WINAPI WorkerThread(LPVOID)
{
    if (WaitForSingleObject(
        g_stopEvent,
        5000
    ) != WAIT_TIMEOUT)
    {
        return 0;
    }

    // --------------------------------------------------------
    // Startup logging
    // --------------------------------------------------------

    Logger::Log(
        "========================================"
    );

    Logger::Log(
        "CarbonDashSFX starting."
    );

    Logger::Log(
        "========================================"
    );

    Logger::Log(
        "Game directory: " +
        Paths::GameDirectory()
    );

    Logger::Log(
        "Scripts directory: " +
        Paths::ScriptsDirectory()
    );

    Logger::Log(
        "SFX directory: " +
        Paths::SFXDirectory()
    );


    // --------------------------------------------------------
    // Configuration
    // --------------------------------------------------------

    CarbonDashConfig config;

    const std::string configPath =
        Paths::ScriptsDirectory() +
        "\\CarbonDashSFX.ini";


    if (!Config::Load(
        configPath,
        config
    ))
    {
        Logger::Log(
            "Failed to load configuration."
        );

        return 0;
    }


    if (!config.enabled)
    {
        Logger::Log(
            "CarbonDashSFX is disabled."
        );

        return 0;
    }


    if (config.sfx.empty())
    {
        Logger::Log(
            "No SFX entries found."
        );

        return 0;
    }


    // --------------------------------------------------------
    // Audio
    // --------------------------------------------------------

    AudioManager audio;

    if (!audio.Initialize())
    {
        Logger::Log(
            "AudioManager initialization failed."
        );

        return 0;
    }


    // --------------------------------------------------------
    // SFX Manager
    // --------------------------------------------------------

    SFXManager sfxManager;

    if (!sfxManager.Initialize(
        config,
        audio
    ))
    {
        Logger::Log(
            "SFXManager initialization failed."
        );

        audio.Shutdown();

        return 0;
    }


    Logger::Log(
        "Loaded SFX runtime count = " +
        std::to_string(
            sfxManager.GetSFXCount()
        )
    );


    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------

    while (
        WaitForSingleObject(
            g_stopEvent,
            POLL_INTERVAL_MS
        ) == WAIT_TIMEOUT
        )
    {
        sfxManager.Update();
    }


    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    sfxManager.Shutdown();
    audio.Shutdown();

    Logger::Log(
        "CarbonDashSFX stopped."
    );

    return 0;
}


// ============================================================
// DLL entry point
// ============================================================

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID
)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        g_stopEvent =
            CreateEventA(
                nullptr,
                TRUE,
                FALSE,
                nullptr
            );

        if (!g_stopEvent)
            return FALSE;

        g_workerThread =
            CreateThread(
                nullptr,
                0,
                WorkerThread,
                nullptr,
                0,
                nullptr
            );

        if (!g_workerThread)
        {
            CloseHandle(
                g_stopEvent
            );

            g_stopEvent = nullptr;

            return FALSE;
        }
    }

    return TRUE;
}