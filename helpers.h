#pragma once
#include <cmath>
#include <string>
    static std::string GetHumanTimeString(double time_seconds)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%02d:%.2f", (int)floor((double)time_seconds / 60.0), std::floor(((((double)time_seconds / 60.0) - (floor(((double)time_seconds / 60.0)))) * 60) * 100.0) / 100.0);
        return{ buffer };
    }
