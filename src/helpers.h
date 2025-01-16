#pragma once
#include <cmath>
#include <string>
//TODO: STOP USING A STRING DUDE
    static std::string GetHumanTimeString(double time_seconds)
    {
        char buffer[128];
        int minutes=(int)(time_seconds/60);
        double seconds=fmod(time_seconds,60.0);
        snprintf(buffer,128,"%02d:%05.2f",minutes,seconds);
        return{ buffer };
    }
