#include "video_operations.h"
#include <iostream>
#include <thread>


#define AV_CHECK(x, s)    \
    do                    \
    {                     \
        if ((x) < 0)      \
        {                 \
            LOGERROR(s);  \
            return false; \
        }                 \
    } while (0)

#define AV_ASSERT(x, s) \
    if (!x)             \
    {                   \
        LOGERROR(s);    \
        exit(1);        \
    }

#define SKIP_FRAME()              \
    {                             \
        av_packet_unref(Packet); \
        av_frame_unref(Frame);   \
        continue;                 \
    }




int skips = 0;
void ReadVideoMetaData(const char* input_file, video_info& info)
{
//    av_log_set_level(AV_LOG_DEBUG);
    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;

    if (avformat_open_input(&pFormatContext, input_file, NULL, NULL) != 0)
    {
        LOGERROR("Failed to open input file");
        exit(1);
    }

    if (avformat_find_stream_info(pFormatContext, NULL) < 0)
    {
        LOGERROR("Failed to locate video stream");
        exit(1);
    }

    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters* pLocalCodecParameters =
            pFormatContext->streams[i]->codecpar;

        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream_idx = i;

        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
            audio_stream_idx = i;
    }
    if (video_stream_idx == -1)
    {
        LOGERROR("Failed to locate video stream");
        exit(1);
    }
    if (audio_stream_idx == -1)
        LOGERROR("Failed to locate audio stream");

    AVStream* video_stream = pFormatContext->streams[video_stream_idx];
    AVStream* audio_stream = pFormatContext->streams[audio_stream_idx];
    AVCodecParameters* c_params = video_stream->codecpar;

    // Fill out video_info struct
    if(audio_stream_idx!=-1)
    info.AudioTimeBase = audio_stream->time_base;
    info.VideoTimeBase = video_stream->time_base;
    info.frame_rate = video_stream->r_frame_rate;
    
    info.duration = video_stream->duration;
    info.width_resolution = c_params->width;
    info.height_resolution = c_params->height;
    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
}

void PrintAVError(int response,const char* message)
{
    char buffer[256];
    av_make_error_string(buffer, 256, response);
    std::cout<<message << " => "<<buffer<<"\n";
    
}

bool OpenVideoStream(const char* input_file, AVStreamInfo& stream_info)
{
    av_log_set_level(AV_LOG_DEBUG);  // Set to AV_LOG_DEBUG for detailed output

    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;

    AV_CHECK(avformat_open_input(&pFormatContext, input_file, NULL, NULL),
        "Failed to open input file");

    avformat_find_stream_info(pFormatContext, NULL);

    int video_stream_idx = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters* pLocalCodecParameters =
            pFormatContext->streams[i]->codecpar;
        if (pFormatContext->streams[i]->codecpar->codec_type ==
            AVMEDIA_TYPE_VIDEO)
            video_stream_idx = i;


    }

    if (video_stream_idx == -1)
    {
        LOGERROR("Failed to locate video stream");
        exit(1);
    }
   
    auto& video_codec_parameters =
        pFormatContext->streams[video_stream_idx]->codecpar;

    const AVCodec* pLocalCodec =
        avcodec_find_decoder(video_codec_parameters->codec_id);
    pCodecContext = avcodec_alloc_context3(pLocalCodec);
    AV_ASSERT(pCodecContext, "Failed to allocate context");

    AV_CHECK(avcodec_parameters_to_context(pCodecContext, video_codec_parameters),
        "Failed to initialize Codec context");

    AV_CHECK(avcodec_open2(pCodecContext, pLocalCodec, NULL),
        "Failed to open codec");


    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();

    AV_ASSERT(pPacket, "Failed to allocate packet");
    AV_ASSERT(pFrame, "Failed to allocate frame");
    stream_info.VideoPacket = pPacket;
    stream_info.VideoFrame = pFrame;
    stream_info.VideoCodecContext = pCodecContext;
    stream_info.VideoFormatContext = pFormatContext;
    stream_info.VideoStreamIndex = video_stream_idx;
    return true;
}

bool OpenAudioStream(const char* input_file, AVStreamInfo& stream_info)
{

    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;

    AV_CHECK(avformat_open_input(&pFormatContext, input_file, NULL, NULL),
        "Failed to open input file");

    avformat_find_stream_info(pFormatContext, NULL);

    int audio_stream_idx = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters* pLocalCodecParameters =
            pFormatContext->streams[i]->codecpar;


        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
            audio_stream_idx = i;
    }


    if (audio_stream_idx == -1)
    {
        LOGERROR("Failed to locate audio stream");
        return false;
    }
    auto& audio_codec_parameters =
        pFormatContext->streams[audio_stream_idx]->codecpar;


    const AVCodec* audio_codec =
        avcodec_find_decoder(audio_codec_parameters->codec_id);
    AVCodecContext* audio_context = avcodec_alloc_context3(audio_codec);

    AV_CHECK(avcodec_parameters_to_context(audio_context, audio_codec_parameters),
        "Failed to initialize audio codec context");

    AV_CHECK(avcodec_open2(audio_context, audio_codec, NULL),
        "Failed to open audio codec");
    


    std::cout << "Created audio codec context\n";

    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();

    AV_ASSERT(pPacket, "Failed to allocate packet");
    AV_ASSERT(pFrame, "Failed to allocate frame");

    stream_info.AudioPacket = pPacket;
    stream_info.AudioFrame = pFrame;
    stream_info.AudioCodecContext = audio_context;
    stream_info.AudioStreamIndex = audio_stream_idx;
    stream_info.AudioFormatContext = pFormatContext;
    
    return true;
}






static SwsContext* sws_context;
static bool first_run = true;
void ReadNextVideoFrame(int timestamp, AVStreamInfo& stream_info, uint8_t* pixel_data)
{
    int response;
    auto FormatContext = stream_info.VideoFormatContext;
    auto CodecContext = stream_info.VideoCodecContext;
    auto Packet = stream_info.VideoPacket;
    auto Frame = stream_info.VideoFrame;


    while (av_read_frame(FormatContext, Packet) >= 0)
    {
        int response = 0;
        if (Packet->stream_index != stream_info.VideoStreamIndex)
        {
            SKIP_FRAME();
        }
       
        response = avcodec_send_packet(CodecContext, Packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            SKIP_FRAME();
        }
      
        if (response < 0)
        {
            PrintAVError(response,"Failed to send packet");
            SKIP_FRAME();
        }
       
        response = avcodec_receive_frame(CodecContext, Frame);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            SKIP_FRAME();
        }
        if (response < 0)
        {

            PrintAVError(response, "Failed to recieve frame");
            SKIP_FRAME();
        }
        if (Frame->pts == AV_NOPTS_VALUE)
        {
            std::cerr << "Frame PTS is invalid\n";
            SKIP_FRAME();
        }
        //FRAME IS VALID

        stream_info.current_timestamp = Frame->pts;
        if(Frame->pts<timestamp)
        {
            SKIP_FRAME();
        }


       
        
        if (first_run)
        {
            sws_context = sws_getContext(
                Frame->width, Frame->height, CodecContext->pix_fmt,
                Frame->width, Frame->height, AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL,
                NULL, NULL);
            first_run = false;
        }


        uint8_t* dest[4] = { pixel_data, NULL, NULL, NULL };
        const int dest_linesize[4] = {Frame->width * 4, 0, 0, 0 };

        sws_scale(sws_context,Frame->data,Frame->linesize, 0,Frame->height,
            dest, dest_linesize);

        break;

    }
    skips = 0;
    stream_info.last_video_pts = Frame->pts;
}
bool ReadFrameSeek(int timestamp, AVStreamInfo& stream_info,
                   //TODO Doesn't Even Get here?
    uint8_t* pixel_data, bool precision_mode)
{
    std::cout<<"Entering ReadFrameSeek";

    auto FormatContext = stream_info.VideoFormatContext;
    auto CodecContext = stream_info.VideoCodecContext;
    auto Packet = stream_info.VideoPacket;
    auto Frame = stream_info.VideoFrame;


    int response = 0;

    // Get the closest keyframe for decoding purposes. Save the timestamp, and
    // nudge up to it
    CodecContext->thread_count = std::thread::hardware_concurrency();
    CodecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    // Clear decoder state before seeking
    avcodec_flush_buffers(CodecContext);

    // Add SEEK_FLAG_FRAME to help find clean entry points
    response =
        av_seek_frame(FormatContext, stream_info.VideoStreamIndex,
            timestamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);


    if (response < 0)
    {
        PrintAVError(response,"Error seeking frame");
        return false;
    }
    while (av_read_frame(FormatContext, Packet) >= 0)
    {
        int response = 0;
        if (Packet->stream_index != stream_info.VideoStreamIndex)
        {
            SKIP_FRAME();
        }
        response = avcodec_send_packet(CodecContext, Packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            SKIP_FRAME();
        }
        if (response < 0)
        {
            avcodec_flush_buffers(CodecContext);
            PrintAVError(response,"Failed to send packet");
            SKIP_FRAME();
        }
        response = avcodec_receive_frame(CodecContext, Frame);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            SKIP_FRAME();
        }
        if (response < 0)
        {
            avcodec_flush_buffers(CodecContext);
            PrintAVError(response,"Failed to recieve frame");
            SKIP_FRAME();
        }

        if (precision_mode)
        {
            // If precision mode, nudge up to the exact timestamp, otherwise select
            // the nearest keyframe
            if (Frame->pts < timestamp)
                SKIP_FRAME();
        }
        stream_info.current_timestamp = Frame->pts;
        if (first_run)
        {
            sws_context = sws_getContext(
                Frame->width, Frame->height, CodecContext->pix_fmt,
                Frame->width, Frame->height, AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL,
                NULL, NULL);
            first_run = false;
        }

        uint8_t* dest[4] = { pixel_data, NULL, NULL, NULL };
        const int dest_linesize[4] = { Frame->width * 4, 0, 0, 0 };

        sws_scale(sws_context, Frame->data, Frame->linesize, 0, Frame->height,
            dest, dest_linesize);

        std::cout<<"Processed Frame";

        break;
    }
    skips = 0;
    stream_info.last_video_pts = Frame->pts;

    return true;
}

int ReadInitialAudioFrame(int timestamp, AVStreamInfo& stream_info, int16_t* audio_data, bool precision_mode)
{

    auto FormatContext = stream_info.AudioFormatContext;
    auto CodecContext = stream_info.AudioCodecContext;
   


    int samples_read = 0;
    AVPacket* Packet = av_packet_alloc();
    AVFrame* Frame = av_frame_alloc();
    





    int response = 0;
    response = av_seek_frame(FormatContext, stream_info.AudioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    if (response < 0)
    {
        PrintAVError(response,"Error seeking frame");
        exit(1);
    }

    int last_frame_read = 0;
    while (av_read_frame(FormatContext, Packet) >= 0)
    {
        if (Packet->stream_index == stream_info.AudioStreamIndex)
        {
            int response = avcodec_send_packet(stream_info.AudioCodecContext, Packet);
            if (response < 0)
            {
                SKIP_FRAME();
            }

            response = avcodec_receive_frame(stream_info.AudioCodecContext, Frame);

            last_frame_read++;

            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            {

                SKIP_FRAME();
            }

            if (precision_mode)
            {

                if (Frame->pts < timestamp)
                    SKIP_FRAME();
            }

            int channel_count = Frame->ch_layout.nb_channels;
            int num_samples = Frame->nb_samples * channel_count;

            if (Frame->format == AV_SAMPLE_FMT_S16)
            {
                std::cout << "Frames in proper format for openAL already!";
                memcpy(audio_data,
                    Frame->data[0],
                    num_samples * sizeof(int16_t));
                break;

            }

            else
            {
                SwrContext* swr_ctx = swr_alloc();
                if (!swr_ctx)
                {
                    LOGERROR("Failed to allocate SwrContext");
                    SKIP_FRAME();
                }

                swr_alloc_set_opts2(
                    &swr_ctx,
                    &Frame->ch_layout,
                    AV_SAMPLE_FMT_S16,
                    Frame->sample_rate,
                    &Frame->ch_layout,
                    (AVSampleFormat)Frame->format,
                    Frame->sample_rate,
                    0, NULL);

                if (swr_init(swr_ctx) < 0)
                {
                    LOGERROR("Failed to initialize SwrContext");
                    swr_free(&swr_ctx);
                    SKIP_FRAME();
                }

                uint8_t* output_buffer = (uint8_t*)(audio_data);
                int converted = swr_convert(
                    swr_ctx,
                    &output_buffer,
                    Frame->nb_samples,
                    (const uint8_t**)Frame->data,
                    Frame->nb_samples);

                if (converted > 0)
                {
                    samples_read = converted * channel_count;
                }

                swr_free(&swr_ctx);
                break;
            }

            av_frame_unref(Frame);
        }
    }
    stream_info.last_audio_pts = Frame->pts;
    av_packet_unref(Packet);
    av_frame_free(&Frame);
    av_packet_free(&Packet);

    return samples_read;
}
int16_t* ReadNextAudioFrame(AVStreamInfo& stream_info, bool precision_mode)
{
    auto FormatContext = stream_info.AudioFormatContext;
    auto CodecContext = stream_info.AudioCodecContext;

    int channels = CodecContext->ch_layout.nb_channels;
    int sample_rate = CodecContext->sample_rate;
    int buffer_size = 10 * sample_rate * channels;
    int16_t* audio_data = new int16_t[buffer_size];
    int samples_read = 0;
    AVPacket* Packet = av_packet_alloc();
    AVFrame* Frame = av_frame_alloc();

    int response = 0;
    response = av_seek_frame(FormatContext, stream_info.AudioStreamIndex, stream_info.last_audio_pts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    if (response < 0)
    {
        PrintAVError(response,"Error seeking frame");
        exit(1);
    }

    int last_frame_read = 0;
    while (av_read_frame(FormatContext, Packet) >= 0)
    {
        if (Packet->stream_index != stream_info.AudioStreamIndex)
        {
            SKIP_FRAME();
        }

        int response = avcodec_send_packet(CodecContext, Packet);
        if (response < 0)
        {
            SKIP_FRAME();
        }

        response = avcodec_receive_frame(CodecContext, Frame);


        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {

            SKIP_FRAME();
        }

        if (Frame->pts <= stream_info.last_audio_pts)
        {
            SKIP_FRAME();
        }


        int channel_count = Frame->ch_layout.nb_channels;
        int num_samples = Frame->nb_samples * channel_count;

        if (Frame->format == AV_SAMPLE_FMT_S16)
        {
            std::cout << "Frames in proper format for openAL already!";
            memcpy(audio_data,
                Frame->data[0],
                num_samples * sizeof(int16_t));
            break;
        }

        else
        {
            SwrContext* swr_ctx = swr_alloc();
            if (!swr_ctx)
            {
                LOGERROR("Failed to allocate SwrContext");
                SKIP_FRAME();
            }

            swr_alloc_set_opts2(
                &swr_ctx,
                &Frame->ch_layout,
                AV_SAMPLE_FMT_S16,
                Frame->sample_rate,
                &Frame->ch_layout,
                (AVSampleFormat)Frame->format,
                Frame->sample_rate,
                0, NULL);

            if (swr_init(swr_ctx) < 0)
            {
                LOGERROR("Failed to initialize SwrContext");
                swr_free(&swr_ctx);
                SKIP_FRAME();
            }

            uint8_t* output_buffer = (uint8_t*)(audio_data);
            int converted = swr_convert(
                swr_ctx,
                &output_buffer,
                Frame->nb_samples,
                (const uint8_t**)Frame->data,
                Frame->nb_samples);

            if (converted > 0)
            {
                samples_read = converted * channel_count;
            }

            swr_free(&swr_ctx);
            break;
        }



    }
    stream_info.last_audio_pts = Frame->pts;
    av_frame_unref(Frame);
    av_packet_unref(Packet);
    av_frame_free(&Frame);
    av_packet_free(&Packet);

    return audio_data;
}

void CloseVideoStream(AVStreamInfo& stream_info)
{
    sws_freeContext(sws_context);
    auto VideoFormatContext = stream_info.VideoFormatContext;
    auto VideoCodecContext = stream_info.VideoCodecContext;
    

    if (VideoFormatContext)
    {
        avformat_close_input(&VideoFormatContext);
        avformat_free_context(VideoFormatContext);
    }
  
    if (VideoCodecContext)
        avcodec_free_context(&VideoCodecContext);
  
   
    std::cout << "Closed video stream\n";
}

void CloseAudioStream(AVStreamInfo& stream_info)
{
    sws_freeContext(sws_context);
   
    auto AudioFormatContext = stream_info.AudioFormatContext;
    auto AudioCodecContext = stream_info.AudioCodecContext;

    
    if (AudioFormatContext)
    {
        avformat_close_input(&AudioFormatContext);
        avformat_free_context(AudioFormatContext);
    }
    
    if (AudioCodecContext)
        avcodec_free_context(&AudioCodecContext);
    
    std::cout << "Closed audio stream\n";
}
