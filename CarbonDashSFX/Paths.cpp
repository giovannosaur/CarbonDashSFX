#include "pch.h"

#include "Paths.h"

#include <windows.h>

namespace Paths
{
    std::string GameDirectory()
    {
        char path[MAX_PATH] = {};

        DWORD length =
            GetModuleFileNameA(nullptr, path, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
            return ".";

        std::string fullPath(path, length);

        size_t slash =
            fullPath.find_last_of("\\/");

        if (slash == std::string::npos)
            return ".";

        return fullPath.substr(0, slash);
    }


    std::string ScriptsDirectory()
    {
        return GameDirectory() + "\\scripts";
    }


    std::string SFXDirectory()
    {
        return ScriptsDirectory() + "\\CarbonDashSFX";
    }
}