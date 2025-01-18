#pragma once
#include <cmath>
#include <string>
#include <cstdio>
#include <cassert>
#include <iostream>
#include "video_operations.h"
static std::string GetHumanTimeString(double time_seconds)
{
    char buffer[15];
    int minutes = (int)(time_seconds / 60);
    double seconds = fmod(time_seconds, 60.0);
    snprintf(buffer, 15, "%02d:%05.2f", minutes, seconds);
    return {buffer};
}

static bool ProcessFilename(const char *inFile, char *outFile)
{

    const char *dot = strrchr(inFile, '.');
    if (!dot)
    {
        std::cerr << "Input file must include extension\n";
        return false;
    }

    size_t nameLen = (size_t)(dot - inFile);

    memcpy(outFile, inFile, nameLen);
    memcpy(outFile + nameLen, "_trimmed", 8);
    strcpy(outFile + nameLen + 8, dot);
    return true;
}

static void PrintUsageError()
{
    std::cerr << "Usage: trim <input_file> [output_file]\n"
                 "  input_file:  Required. The file to process ( must include "
                 "extension)\n"
                 "  output_file: Optional. The destination file (if omitted, "
                 "will be auto-generated)\n";
}

static uint32_t ToPTS(double seconds,AVRational& timebase)
{
    uint32_t f = std::floor((double)timebase.den/(double)timebase.num);
    return seconds*f;
}
static double ToSeconds(uint32_t pts,AVRational& timebase)
{
    double f= ((double)timebase.num/(double)timebase.den);
    return (double)pts*f;
}