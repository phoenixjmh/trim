#pragma once
#include <cstdint>

namespace GUI
{
    struct Image
    {
        uint32_t ID;
        int width;
        int height;
        int channels;
    };

    Image LoadTexture(const char *filename);
} // GUI
