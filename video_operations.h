#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#define LOGERROR(x) std::cerr<<"Error :: " <<x<<"\n"
struct video_info
{
    int width_resolution;
    int height_resolution;
    uint32_t frame_count;
    uint32_t duration;
    AVRational time_base;
};

struct AVStreamInfo
{
    AVFormatContext* pFormatContext = nullptr;
    AVCodecContext* pCodecContext = nullptr;
    int video_stream_index = -1;
};

bool OpenVideoStream(const char* input_file, AVStreamInfo& stream_info);

bool ReadFrameFromOpenStream(int timestamp, AVStreamInfo& stream_info, uint8_t* pixel_data,bool precision_mode);

void CloseVideoStream(AVStreamInfo& stream_info);

void ReadVideoMetaData(const char* input_file, video_info& info);

//bool ReadFrame(const char* input_file, uint8_t* pixel_data,int64_t timestamp);

