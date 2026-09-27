#include "pch.h"

#include "Logger.h"
#include "Paths.h"

#include <fstream>

namespace Logger
{
    void Log(const std::string& message)
    {
        static std::ofstream logFile(
            Paths::ScriptsDirectory() +
            "\\CarbonDashSFX.log",
            std::ios::trunc
        );

        logFile << message << std::endl;
    }
}