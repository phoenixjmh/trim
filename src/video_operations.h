#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define LOGERROR(x) std::cerr<<"Error :: " <<x<<"\n"
struct video_info
{
    int width_resolution;
    int height_resolution;
    uint32_t frame_count;
    uint32_t duration;
    AVRational VideoTimeBase;
    AVRational AudioTimeBase;
    AVRational frame_rate;
};

struct audio_info
{
    AVRational VideoTimeBase;
};

struct AVStreamInfo
{
    AVFormatContext* AudioFormatContext = nullptr;
    AVFormatContext* VideoFormatContext = nullptr;
    AVCodecContext* VideoCodecContext = nullptr;
    AVCodecContext* AudioCodecContext=nullptr;
    AVPacket* VideoPacket=nullptr;
    AVFrame* VideoFrame = nullptr;
    AVPacket* AudioPacket = nullptr;
    AVFrame* AudioFrame = nullptr;
    int VideoStreamIndex = -1;
    int AudioStreamIndex = -1;
    uint32_t current_timestamp = 0;
    uint32_t last_audio_pts=0;
    uint32_t last_video_pts = 0;
    
};

bool OpenVideoStream(const char* input_file, AVStreamInfo& stream_info);
bool OpenAudioStream(const char* input_file, AVStreamInfo& stream_info);

bool ReadFrameSeek(int timestamp, AVStreamInfo& stream_info, uint8_t* pixel_data,bool precision_mode);

void ReadNextVideoFrame(int timestamp,AVStreamInfo& stream_info, uint8_t* pixel_data);

int ReadInitialAudioFrame(int timestamp, AVStreamInfo &stream_info, int16_t *audio_data, bool precision_mode = true);
int16_t* ReadNextAudioFrame(AVStreamInfo& stream_info,bool precision_mode=true);



void CloseVideoStream(AVStreamInfo& stream_info);
void CloseAudioStream(AVStreamInfo& stream_info);

void ReadVideoMetaData(const char* input_file, video_info& info);


