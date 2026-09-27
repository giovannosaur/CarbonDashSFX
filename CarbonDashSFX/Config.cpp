#include "pch.h"

#include "Config.h"
#include "Logger.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <utility>


namespace
{
    std::string Trim(
        const std::string& value
    )
    {
        size_t start = 0;

        while (
            start < value.size() &&
            std::isspace(
                static_cast<unsigned char>(
                    value[start]
                    )
            )
            )
        {
            ++start;
        }


        size_t end = value.size();

        while (
            end > start &&
            std::isspace(
                static_cast<unsigned char>(
                    value[end - 1]
                    )
            )
            )
        {
            --end;
        }


        return value.substr(
            start,
            end - start
        );
    }


    std::string ToLower(
        std::string value
    )
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](
                unsigned char c
                )
            {
                return static_cast<char>(
                    std::tolower(c)
                    );
            }
        );

        return value;
    }


    std::vector<std::string> SplitCommaList(
        const std::string& value
    )
    {
        std::vector<std::string> result;

        std::stringstream stream(value);

        std::string item;

        while (std::getline(stream, item, ','))
        {
            item = Trim(item);

            if (!item.empty())
            {
                result.push_back(item);
            }
        }

        return result;
    }


    bool ParseRPMTarget(
        const std::string& input,
        RPMTarget& output
    )
    {
        const std::string value =
            Trim(input);

        if (
            value.empty() ||
            value == "0"
            )
        {
            output.type =
                RPMTargetType::None;

            output.modifier = 0.0f;
            output.fixedValue = 0.0f;

            return true;
        }


        const std::string lower =
            ToLower(value);


        // ----------------------------------------------------
        // Fixed RPM
        //
        // Example:
        // 7000
        // ----------------------------------------------------

        char* fixedEnd = nullptr;

        float fixedValue =
            std::strtof(
                value.c_str(),
                &fixedEnd
            );


        if (
            fixedEnd != value.c_str() &&
            *fixedEnd == '\0'
            )
        {
            output.type =
                RPMTargetType::Fixed;

            output.fixedValue =
                fixedValue;

            output.modifier = 0.0f;

            return true;
        }


        // ----------------------------------------------------
        // Determine target type
        // ----------------------------------------------------

        RPMTargetType type;

        if (
            lower.rfind(
                "redline",
                0
            ) == 0
            )
        {
            type =
                RPMTargetType::Redline;
        }
        else if (
            lower.rfind(
                "maxrpm",
                0
            ) == 0
            )
        {
            type =
                RPMTargetType::MaxRPM;
        }
        else
        {
            return false;
        }


        output.type = type;
        output.modifier = 0.0f;
        output.fixedValue = 0.0f;


        // ----------------------------------------------------
        // No modifier
        //
        // redline
        // maxrpm
        // ----------------------------------------------------

        const size_t baseLength =
            type == RPMTargetType::Redline
            ? 7
            : 6;


        if (lower.size() == baseLength)
        {
            return true;
        }


        // ----------------------------------------------------
        // Modifier
        //
        // redline(-500)
        // redline(+400)
        // maxrpm(-200)
        // ----------------------------------------------------

        const size_t open =
            value.find('(');

        const size_t close =
            value.find(')');


        if (
            open == std::string::npos ||
            close == std::string::npos ||
            close <= open
            )
        {
            return false;
        }


        // No extra characters after ')'
        if (close != value.size() - 1)
        {
            return false;
        }


        const std::string modifierText =
            value.substr(
                open + 1,
                close - open - 1
            );


        char* modifierEnd = nullptr;

        float modifier =
            std::strtof(
                modifierText.c_str(),
                &modifierEnd
            );


        if (
            modifierEnd ==
            modifierText.c_str() ||
            *modifierEnd != '\0'
            )
        {
            return false;
        }


        output.modifier = modifier;

        return true;
    }


    bool ParseTargetSpeed(
        const std::string& input,
        float& output
    )
    {
        const std::string value =
            Trim(input);

        if (value.empty())
        {
            output = 0.0f;
            return true;
        }


        char* end = nullptr;

        float result =
            std::strtof(
                value.c_str(),
                &end
            );


        if (
            end == value.c_str() ||
            *end != '\0'
            )
        {
            return false;
        }


        output = result;

        return true;
    }
}


namespace Config
{
    bool Load(
        const std::string& path,
        CarbonDashConfig& config
    )
    {
        config =
            CarbonDashConfig{};


        // ----------------------------------------------------
        // Global settings
        // ----------------------------------------------------

        config.enabled =
            GetPrivateProfileIntA(
                "CarbonDashSFX",
                "Enabled",
                1,
                path.c_str()
            ) != 0;


        // ----------------------------------------------------
        // Get all section names
        // ----------------------------------------------------

        char sectionBuffer[8192] = {};

        DWORD length =
            GetPrivateProfileSectionNamesA(
                sectionBuffer,
                sizeof(sectionBuffer),
                path.c_str()
            );


        if (length == 0)
        {
            Logger::Log(
                "Config: failed to read sections from " +
                path
            );

            return false;
        }


        const char* current =
            sectionBuffer;


        while (*current != '\0')
        {
            const std::string section =
                current;


            current +=
                section.size() + 1;


            // Ignore global section.
            if (section == "CarbonDashSFX")
            {
                continue;
            }


            // Only process [SFX...]
            if (
                section.size() < 4 ||
                section.rfind("SFX", 0) != 0
                )
            {
                continue;
            }


            SFXConfig sfx;


            // ------------------------------------------------
            // Type
            // ------------------------------------------------

            sfx.type =
                GetPrivateProfileIntA(
                    section.c_str(),
                    "Type",
                    0,
                    path.c_str()
                );


            if (
                sfx.type < 0 ||
                sfx.type > 2
                )
            {
                Logger::Log(
                    "Config: invalid Type in [" +
                    section +
                    "]"
                );

                continue;
            }


            // ------------------------------------------------
            // Volume
            // ------------------------------------------------

            sfx.volume =
                GetPrivateProfileIntA(
                    section.c_str(),
                    "Vol",
                    100,
                    path.c_str()
                );


            if (
                sfx.volume < 0 ||
                sfx.volume > 100
                )
            {
                Logger::Log(
                    "Config: invalid Vol in [" +
                    section +
                    "]"
                );

                continue;
            }


            char buffer[4096] = {};


            // ------------------------------------------------
            // TargetRPM
            // ------------------------------------------------

            GetPrivateProfileStringA(
                section.c_str(),
                "TargetRPM",
                "0",
                buffer,
                sizeof(buffer),
                path.c_str()
            );


            if (
                !ParseRPMTarget(
                    buffer,
                    sfx.rpmTarget
                )
                )
            {
                Logger::Log(
                    "Config: invalid TargetRPM in [" +
                    section +
                    "]: " +
                    buffer
                );

                continue;
            }


            // ------------------------------------------------
            // TargetSpeed
            // ------------------------------------------------

            GetPrivateProfileStringA(
                section.c_str(),
                "TargetSpeed",
                "0",
                buffer,
                sizeof(buffer),
                path.c_str()
            );


            if (
                !ParseTargetSpeed(
                    buffer,
                    sfx.targetSpeed
                )
                )
            {
                Logger::Log(
                    "Config: invalid TargetSpeed in [" +
                    section +
                    "]: " +
                    buffer
                );

                continue;
            }


            // ------------------------------------------------
            // File
            // ------------------------------------------------

            GetPrivateProfileStringA(
                section.c_str(),
                "File",
                "",
                buffer,
                sizeof(buffer),
                path.c_str()
            );


            sfx.file =
                Trim(buffer);


            if (sfx.file.empty())
            {
                Logger::Log(
                    "Config: missing File in [" +
                    section +
                    "]"
                );

                continue;
            }


            // ------------------------------------------------
            // TargetVehicle
            // ------------------------------------------------

            GetPrivateProfileStringA(
                section.c_str(),
                "TargetVehicle",
                "",
                buffer,
                sizeof(buffer),
                path.c_str()
            );


            sfx.targetVehicles =
                SplitCommaList(buffer);


            // ------------------------------------------------
            // Store SFX
            // ------------------------------------------------

            config.sfx.push_back(
                std::move(sfx)
            );
        }


        Logger::Log(
            "Config loaded successfully."
        );

        Logger::Log(
            "Enabled = " +
            std::to_string(
                config.enabled ? 1 : 0
            )
        );

        Logger::Log(
            "SFX count = " +
            std::to_string(
                config.sfx.size()
            )
        );


        return true;
    }
}