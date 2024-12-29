#include "video_operations.h"
#include <iostream>


bool ReadVideoMetaData(const char* input_file, video_info& info)
{
    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;



    if (avformat_open_input(&pFormatContext, input_file, NULL, NULL) != 0)
    {
        std::cerr << "Couldn't open input file!\n";
        return false;
    }

    avformat_find_stream_info(pFormatContext, NULL);


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
        std::cerr<<"Failed to find video stream\n";
        return false;
    }

    AVStream* video_stream = pFormatContext->streams[video_stream_idx];
    AVCodecParameters* c_params = video_stream->codecpar;

    //Fill out video_info struct
    info.time_base = video_stream->time_base;
    info.duration = video_stream->duration;
    info.width_resolution = c_params->width;
    info.height_resolution = c_params->height;
    
    return true;
}

bool ReadFrame(const char* input_file, uint8_t* pixel_data,int64_t timestamp)
{
    AVFormatContext* pFormatContext = avformat_alloc_context();
    AVCodecContext* pCodecContext = nullptr;

    if (avformat_open_input(&pFormatContext, input_file, NULL, NULL) != 0)
    {
        return false;
    }

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
        exit(1);
    }
    auto& video_codec_parameters = pFormatContext->streams[video_stream_idx]->codecpar;

    const AVCodec* pLocalCodec = avcodec_find_decoder(video_codec_parameters->codec_id);
    pCodecContext = avcodec_alloc_context3(pLocalCodec);
    if (!pCodecContext)
    {
        std::cerr<<("Failed to allocate context!\n");
        return false;
    }
    if (avcodec_parameters_to_context(pCodecContext, video_codec_parameters) < 0)
    {
        std::cerr<<("Couldn't initialize CodecContext\n");
        return false;
    }
    if (avcodec_open2(pCodecContext, pLocalCodec, NULL) < 0)
    {
        std::cerr<<("Couldn't open codec\n");
        return false;
    }
    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    if (!pPacket)
    {
        std::cerr<<("Failed to allocate packet!\n");
        return false;
    }
    if (!pFrame)
    {
        std::cerr<<("Failed to allocate frame!\n");
        return false;
    }





    int response = 0;
    //Get the closest keyframe for decoding purposes. Save the timestamp, and nudge up to it
    response = av_seek_frame(pFormatContext, video_stream_idx, timestamp, AVSEEK_FLAG_BACKWARD);
    if (response < 0)
    {
        char err_buff[256];
        av_make_error_string(err_buff, 256, response);
        std::cerr<<"Error seeking frame =>%s", err_buff;
    }
    //ITERATE THROUGH FRAMES
    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        int response = 0;
        if (pPacket->stream_index != video_stream_idx)
        {
            av_packet_unref(pPacket);
            av_frame_unref(pFrame);
            continue;
        }
        response = avcodec_send_packet(pCodecContext, pPacket);
        if (response < 0)
        {
            char err_buff[256];
            av_make_error_string(err_buff, 256, response);
            std::cerr<<"Failed to decode packet => %s\n", err_buff;
            av_packet_unref(pPacket);
            av_frame_unref(pFrame);
            continue;
        }
        response = avcodec_receive_frame(pCodecContext, pFrame);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            av_packet_unref(pPacket);
            av_frame_unref(pFrame);
            continue;
        }
         if (response < 0)
        {
            char err_buff[256];
            av_make_error_string(err_buff, 256, response);
            std::cerr<<"Failed to decode packet: => "<< err_buff<<"\n";
            continue;
        }
        //Nudge up to the desired timestamp
        if (pFrame->pts < timestamp)
        {
            av_packet_unref(pPacket);
            av_frame_unref(pFrame);
            continue;
        }
       
      
        


        SwsContext* sws_context =
            sws_getContext(pFrame->width, pFrame->height, pCodecContext->pix_fmt, pFrame->width, pFrame->height,
                AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);

        //uint8_t* data = new uint8_t[pFrame->width * pFrame->height * 4];
        uint8_t* dest[4] = { pixel_data, NULL, NULL, NULL };
        const int dest_linesize[4] = { pFrame->width * 4, 0, 0, 0 };

        sws_scale(sws_context, pFrame->data, pFrame->linesize, 0, pFrame->height, dest, dest_linesize);

        sws_freeContext(sws_context);
        break;
    }
    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
    avcodec_free_context(&pCodecContext);
       av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    return true;
}
