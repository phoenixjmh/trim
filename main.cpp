#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include <iostream>
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
};

bool ReadVideoMetaData(const char *input_file, video_info &info)
{
    AVFormatContext *pFormatContext = avformat_alloc_context();
    AVCodecContext *pCodecContext = nullptr;

    if (avformat_open_input(&pFormatContext, input_file, NULL, NULL) != 0)
    {
        return false;
    }
    printf("Format %s, duration %ld us\n", pFormatContext->iformat->long_name, pFormatContext->duration);

    avformat_find_stream_info(pFormatContext, NULL);

    int video_stream_idx = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
            video_stream_idx = i;
            pCodecContext = avcodec_alloc_context3(pLocalCodec);
            avcodec_parameters_to_context(pCodecContext, pLocalCodecParameters);
            avcodec_open2(pCodecContext, pLocalCodec, NULL);
        }
    }
    if (video_stream_idx == -1)
    {
        printf("Failed to find video stream\n");
        return false;
    }

    //Get time base
    auto tbase=pFormatContext->streams[video_stream_idx]->time_base;



    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    if (!pPacket)
    {
        printf("Failed to allocate packet!");
        return false;
    }

    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        if (pPacket->stream_index != video_stream_idx)
            continue;
        avcodec_send_packet(pCodecContext, pPacket);
        avcodec_receive_frame(pCodecContext, pFrame);

        if (pCodecContext->frame_num == 1)
        {
            info.width_resolution = pFrame->width;
            info.height_resolution = pFrame->height;
            avformat_close_input(&pFormatContext);
            avformat_free_context(pFormatContext);
            av_frame_free(&pFrame);
            av_packet_free(&pPacket);
            avcodec_free_context(&pCodecContext);
            break;
        }
    }


    return true;
}

bool ReadFrame(const char *input_file, uint8_t **pixel_data, int frame_index)
{

    AVFormatContext *pFormatContext = avformat_alloc_context();
    AVCodecContext *pCodecContext = nullptr;

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

        if (video_stream_idx==-1)
        {
            exit(1);
        }
        auto& video_codec_parameters=pFormatContext->streams[video_stream_idx]->codecpar;

        const AVCodec* pLocalCodec=avcodec_find_decoder(video_codec_parameters->codec_id);
        pCodecContext = avcodec_alloc_context3(pLocalCodec);
        if (!pCodecContext)
        {
            printf("Failed to allocate context!\n");
            return false;
        }
        if (avcodec_parameters_to_context(pCodecContext, video_codec_parameters)<0)
        {
            printf("Couldn't initialize CodecContext\n");
            return false;
        }
        if (avcodec_open2(pCodecContext, pLocalCodec, NULL)<0)
        {
            printf("Couldn't open codec\n");
            return false;
        }
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    if (!pPacket)
    {
        printf("Failed to allocate packet!\n");
        return false;
    }
    if (!pFrame)
    {
        printf("Failed to allocate frame!\n");
        return false;
    }




    //SEEK DESIRED FRAME

    int response=0;
    response=av_seek_frame(pFormatContext,video_stream_idx,8006999,AVSEEK_FLAG_BACKWARD);
    if (response<0)
    {
        char err_buff[256];
        av_make_error_string(err_buff,256,response);
        printf("Error seeking frame =>%s",err_buff);
    }
    //ITERATE THROUGH FRAMES
    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        int response=0;
        if (pPacket->stream_index != video_stream_idx)
        {
            av_packet_unref(pPacket);
            continue;
        }
        response=avcodec_send_packet(pCodecContext, pPacket);
        if (response<0)
        {
            char err_buff[256];
            av_make_error_string(err_buff,256,response);
            printf("Failed to decode packet => %s\n",err_buff);
            av_packet_unref(pPacket);
            continue;
        }
        response = avcodec_receive_frame(pCodecContext, pFrame);

        if (response==AVERROR(EAGAIN)||response==AVERROR_EOF)
        {
            av_packet_unref(pPacket);
            continue;
        }
        else if (response<0)
        {
            char err_buff[256];
            av_make_error_string(err_buff,256,response);
            printf("Failed to decode packet: => %s\n",err_buff);
            continue;
        }

        //Presentation time stamp
        //pFrame->pts
            auto time_stamp=pFrame->pts;



            std::cout<<"Time_stamp =>"<<time_stamp<<"\n";


            SwsContext *sws_context =
                sws_getContext(pFrame->width, pFrame->height, pCodecContext->pix_fmt, pFrame->width, pFrame->height,
                               AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);

            uint8_t *data = new uint8_t[pFrame->width * pFrame->height * 4];
            uint8_t *dest[4] = {data, NULL, NULL, NULL};
            const int dest_linesize[4] = {pFrame->width * 4, 0, 0, 0};

            sws_scale(sws_context, pFrame->data, pFrame->linesize, 0, pFrame->height, dest, dest_linesize);

            *pixel_data = data;
            break;
    }
    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    avcodec_free_context(&pCodecContext);
    return true;
}

int main()
{

    GLFWwindow *window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(1280, 720, "Trim", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Error => " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return -1;
    }

    uint8_t *pixel_data;

    video_info info{};
    static int frame_idx = 8000;
    const char * input_file="../res/input3.mp4";

    if (!ReadVideoMetaData(input_file, info))
    {
        return -1;
    }
    if (posix_memalign((void**)&pixel_data, 128, info.width_resolution * info.height_resolution * 4) != 0) {
        printf("Couldn't allocate frame buffer\n");
        return 1;
    }

  if (!ReadFrame(input_file, &pixel_data, frame_idx))
        {
            return -1;
        }



    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            if (key == GLFW_KEY_LEFT)
            {
                frame_idx--;
            }
            else if (key == GLFW_KEY_RIGHT)
            {
                frame_idx++;
            }
        }
    });

    while (!glfwWindowShouldClose(window))
    {
        if (frame_idx < 0)
            frame_idx = 0;


        GLuint hTex;
        glGenTextures(1, &hTex);
        glBindTexture(GL_TEXTURE_2D, hTex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width_resolution, info.height_resolution, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, pixel_data);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        int windowW, windowH;
        glfwGetWindowSize(window, &windowW, &windowH);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowW, windowH, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, hTex);
        glBegin(GL_QUADS);
        glTexCoord2d(0, 0);
        glVertex2i(0, 0);
        glTexCoord2d(1, 0);
        glVertex2i(windowW, 0);
        glTexCoord2d(1, 1);
        glVertex2i(windowW, windowH);
        glTexCoord2d(0, 1);
        glVertex2i(0, windowH);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
