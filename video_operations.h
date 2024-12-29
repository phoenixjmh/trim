#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

struct video_info
{
    int width_resolution;
    int height_resolution;
    uint32_t frame_count;
    uint32_t duration;
    AVRational time_base;
};

bool ReadVideoMetaData(const char* input_file, video_info& info);

bool ReadFrame(const char* input_file, uint8_t* pixel_data,int64_t timestamp);

