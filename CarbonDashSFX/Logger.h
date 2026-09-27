#pragma once

#include <string>

namespace Logger
{
    void Log(const std::string& message);
}

#ifdef CARBONDASHSFX_DEBUG

#define LOG_DEBUG(message) \
    Logger::Log(message)

#else

#define LOG_DEBUG(message) \
    do {} while (0)

#endif