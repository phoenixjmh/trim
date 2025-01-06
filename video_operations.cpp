#include <iostream>
#include "video_operations.h"
#include <thread>
#define AV_CHECK(x,s) if(x<0){ LOGERROR(s); return false;}

#define AV_CHECK(x, s) do { \
    if ((x) < 0) { \
        LOGERROR(s); \
        return false; \
    } \
} while(0)

#define AV_ASSERT(x,s) if(!x){LOGERROR(s);exit(1);}

void ReadVideoMetaData(const char* input_file, video_info& info)
{
    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;

    if(avformat_open_input(&pFormatContext, input_file, NULL, NULL)!=0)
    {
        LOGERROR("Failed to open input file");
        exit(1);
    }

    if(avformat_find_stream_info(pFormatContext, NULL)<0)
    {
        LOGERROR("Failed to locate video stream");
        exit(1);
    }


    int video_stream_idx = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters* pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_idx = i;
        }
    }
    if (video_stream_idx == -1)
    {
        LOGERROR("Failed to locate video stream");
        exit(1);
    }

    AVStream* video_stream = pFormatContext->streams[video_stream_idx];
    AVCodecParameters* c_params = video_stream->codecpar;

    //Fill out video_info struct
    info.time_base = video_stream->time_base;
    info.duration = video_stream->duration;
    info.width_resolution = c_params->width;
    info.height_resolution = c_params->height;
    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);

}

const char* GetAVError(int response)
{
    char err_buff[256];
    av_make_error_string(err_buff, 256, response);
    printf("Error %s", err_buff);
    return err_buff;

}

bool OpenVideoStream(const char* input_file,AVStreamInfo& stream_info)
{
    //av_log_set_level(AV_LOG_DEBUG);  // Set to AV_LOG_DEBUG for detailed output




    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;

    AV_CHECK(avformat_open_input(&pFormatContext, input_file, NULL, NULL), "Failed to open input file");


    avformat_find_stream_info(pFormatContext, NULL);

    int video_stream_idx = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_idx = i;
            break;
        }
    }

    if (video_stream_idx == -1)
    {
        LOGERROR("Failed to locate video stream");
        exit(1);
    }
    auto& video_codec_parameters = pFormatContext->streams[video_stream_idx]->codecpar;

    const AVCodec* pLocalCodec = avcodec_find_decoder(video_codec_parameters->codec_id);
    pCodecContext = avcodec_alloc_context3(pLocalCodec);
    AV_ASSERT(pCodecContext, "Failed to allocate context");

    AV_CHECK(avcodec_parameters_to_context(pCodecContext, video_codec_parameters), "Failed to initialize Codec context");

    AV_CHECK(avcodec_open2(pCodecContext, pLocalCodec, NULL), "Failed to open codec");

    stream_info.pCodecContext = pCodecContext;
    stream_info.pFormatContext = pFormatContext;
    stream_info.video_stream_index = video_stream_idx;
    return true;
}


bool ReadFrameFromOpenStream(int timestamp, AVStreamInfo& stream_info,uint8_t* pixel_data,bool precision_mode)
{

    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();

    AV_ASSERT(pPacket, "Failed to allocate packet");

    AV_ASSERT(pFrame, "Failed to allocate frame");

#define SKIP_FRAME(){av_packet_unref(pPacket);av_frame_unref(pFrame);continue;}

    int response = 0;

    //Get the closest keyframe for decoding purposes. Save the timestamp, and nudge up to it
    stream_info.pCodecContext->thread_count = std::thread::hardware_concurrency();
    stream_info.pCodecContext->thread_type = FF_THREAD_FRAME;
    // Clear decoder state before seeking
    avcodec_flush_buffers(stream_info.pCodecContext);

    // Add SEEK_FLAG_FRAME to help find clean entry points
    response = av_seek_frame(stream_info.pFormatContext, stream_info.video_stream_index,
        timestamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    if (response < 0)
    {
        std::cerr << "Error seeking frame =>" << GetAVError(response);
        exit(1);
    }

    while (av_read_frame(stream_info.pFormatContext, pPacket) >= 0)
    {
        int response = 0;
        if (pPacket->stream_index != stream_info.video_stream_index)
        {
            SKIP_FRAME();
        }
        response = avcodec_send_packet(stream_info.pCodecContext, pPacket);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            av_packet_unref(pPacket);
            av_frame_unref(pFrame);

            // Small sleep to prevent tight loop
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // Try receiving frame again before getting new packet
            response = avcodec_receive_frame(stream_info.pCodecContext, pFrame);
            if (response >= 0)
                continue;  // Got a frame, process it

            continue;  // Still no frame, try new packet
        }
        if (response < 0)
        {
            avcodec_flush_buffers(stream_info.pCodecContext);
            std::cerr << "Failed to send packet => "<< GetAVError(response)<<"\n";
            SKIP_FRAME();
        }
        response = avcodec_receive_frame(stream_info.pCodecContext, pFrame);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            SKIP_FRAME();
        }
        if (response < 0)
        {
            avcodec_flush_buffers(stream_info.pCodecContext);
            std::cerr << "Failed to recieve frame: => " << GetAVError(response) << "\n";
            SKIP_FRAME();
        }
        
        if(precision_mode)
        {
            //If precision mode, nudge up to the exact timestamp, otherwise select the nearest keyframe
            if (pFrame->pts < timestamp)
                SKIP_FRAME();
        }

        SwsContext* sws_context =
            sws_getContext(pFrame->width, pFrame->height, stream_info.pCodecContext->pix_fmt, pFrame->width, pFrame->height,
                AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);

        //uint8_t* data = new uint8_t[pFrame->width * pFrame->height * 4];
        uint8_t* dest[4] = { pixel_data, NULL, NULL, NULL };
        const int dest_linesize[4] = { pFrame->width * 4, 0, 0, 0 };

        sws_scale(sws_context, pFrame->data, pFrame->linesize, 0, pFrame->height, dest, dest_linesize);

        sws_freeContext(sws_context);
       
        break;
    }
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
}

void CloseVideoStream(AVStreamInfo& stream_info)
{
    if(stream_info.pFormatContext)
    {
    avformat_close_input(&stream_info.pFormatContext);
    avformat_free_context(stream_info.pFormatContext);
    }
    else{
        std::cout<<("Attempting to close video that is not open\n");
        return;
    }
    if(stream_info.pCodecContext)
    avcodec_free_context(&stream_info.pCodecContext);
    else
    {
        std::cout << ("Attempting to close video that is not open\n");
        return;
    }
    std::cout << "Closed video stream\n";
}



//bool ReadFrame(const char* input_file, uint8_t* pixel_data, int64_t timestamp)
//{
//    AVFormatContext* pFormatContext = avformat_alloc_context();
//    AVCodecContext* pCodecContext = nullptr;
//
//    AV_CHECK(avformat_open_input(&pFormatContext, input_file, NULL, NULL), "Failed to open input file");
//
//
//    avformat_find_stream_info(pFormatContext, NULL);
//
//    int video_stream_idx = -1;
//    for (int i = 0; i < pFormatContext->nb_streams; i++)
//    {
//        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
//        {
//            video_stream_idx = i;
//            break;
//        }
//    }
//
//    if (video_stream_idx == -1)
//    {
//        LOGERROR("Failed to locate video stream");
//        exit(1);
//    }
//    auto& video_codec_parameters = pFormatContext->streams[video_stream_idx]->codecpar;
//
//    const AVCodec* pLocalCodec = avcodec_find_decoder(video_codec_parameters->codec_id);
//    pCodecContext = avcodec_alloc_context3(pLocalCodec);
//    AV_ASSERT(pCodecContext, "Failed to allocate context");
//
//    AV_CHECK(avcodec_parameters_to_context(pCodecContext, video_codec_parameters), "Failed to initialize Codec context");
//
//    AV_CHECK(avcodec_open2(pCodecContext, pLocalCodec, NULL), "Failed to open codec");
//
//    AVPacket* pPacket = av_packet_alloc();
//    AVFrame* pFrame= av_frame_alloc();
//
//    AV_ASSERT(pPacket, "Failed to allocate packet");
//
//    AV_ASSERT(pFrame, "Failed to allocate frame");
//
//#define SKIP_FRAME(){av_packet_unref(pPacket);av_frame_unref(pFrame);continue;}
//
//    int response = 0;
//
//    //Get the closest keyframe for decoding purposes. Save the timestamp, and nudge up to it
//    response = av_seek_frame(pFormatContext, video_stream_idx, timestamp, AVSEEK_FLAG_BACKWARD);
//    if (response < 0)
//    {
//        std::cerr << "Error seeking frame =>%s", GetAVError(response);
//        exit(1);
//    }
//    //ITERATE THROUGH FRAMES
//    while (av_read_frame(pFormatContext, pPacket) >= 0)
//    {
//        int response = 0;
//        if (pPacket->stream_index != video_stream_idx)
//        {
//            SKIP_FRAME();
//        }
//        response = avcodec_send_packet(pCodecContext, pPacket);
//        if (response < 0)
//        {
//            std::cerr << "Failed to decode packet => %s\n", GetAVError(response);
//            SKIP_FRAME();
//        }
//        response = avcodec_receive_frame(pCodecContext, pFrame);
//
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
//        {
//            SKIP_FRAME();
//        }
//        if (response < 0)
//        {
//            std::cerr << "Failed to decode packet: => " << GetAVError(response) << "\n";
//            SKIP_FRAME();
//        }
//        //Nudge up to the desired timestamp
//        if (pFrame->pts < timestamp)
//            SKIP_FRAME();
//
//
//
//
//
//        SwsContext* sws_context =
//            sws_getContext(pFrame->width, pFrame->height, pCodecContext->pix_fmt, pFrame->width, pFrame->height,
//                AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);
//
//        //uint8_t* data = new uint8_t[pFrame->width * pFrame->height * 4];
//        uint8_t* dest[4] = { pixel_data, NULL, NULL, NULL };
//        const int dest_linesize[4] = { pFrame->width * 4, 0, 0, 0 };
//
//        sws_scale(sws_context, pFrame->data, pFrame->linesize, 0, pFrame->height, dest, dest_linesize);
//
//        sws_freeContext(sws_context);
//        break;
//    }
//    avformat_close_input(&pFormatContext);
//    avformat_free_context(pFormatContext);
//    avcodec_free_context(&pCodecContext);
//    av_frame_free(&pFrame);
//    av_packet_free(&pPacket);
//    return true;
//}

