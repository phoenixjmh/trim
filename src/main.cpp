#define _CRT_SECURE_NO_WARNINGS
#include <mutex>
#include <queue>
#include "trim.h"
#include "openAL.h"
namespace fs = std::filesystem;
// [ ]TODO: Apply the content scaling before hand, make the dock_size a non static non global.
// [ ]TODO: Clean up the cmake shit for SURE.
// [ ]TODO: Remove all these static variables probably


//GUI RELATED
// TODO: Precision mode is fucked up. 
// TODO: Get ariel to sign off on changes to the GUI

static glm::mat4 model(1.0f);

static std::string FFMPEG_OUTPUT_COMMAND;
GLuint VAO, VBO;
glm::mat4 proj;

bool renderFirstFrame = true;
static bool Playback = false;
static bool first_playback_frame;

video_info info{};
#define ALSOFT_LOGLEVEL 3
const int ACCUMULATOR_SAMPLE_SIZE = 30;
static double frametime_accumulator = 0;
static int frame_accumulator = 0;

static bool running = false;
static bool has_audio_stream = true;
static bool video_behind = false;
static int dock_height = 0;

double GetFPS(double last_frame_time,double source_fps)
{

    auto frame_start = glfwGetTime();
    auto video_frame_output_time = frame_start - last_frame_time;

    if (frame_accumulator == ACCUMULATOR_SAMPLE_SIZE)
    {
        double ms = (double)frametime_accumulator / ACCUMULATOR_SAMPLE_SIZE;
        int FPS = floor(((double)ACCUMULATOR_SAMPLE_SIZE / (double)frametime_accumulator));
         printf("OUTPUT: FPS=>%d  | Native: %d\n", FPS, (int)source_fps);
        frame_accumulator = 0;
        frametime_accumulator = 0;
    }
    return video_frame_output_time;
}
int main(int argc, char *argv[])
{
    SystemCallParameters callParams = {};
    callParams.trim_start = new uint32_t(1);
    callParams.trim_end = new uint32_t(1);
    ProcessArgs(argc, argv, callParams);
    ALCdevice *audio_device = nullptr;
    ALCcontext *audio_context = nullptr;
    OpenAL::SetupAudio(audio_device, audio_context);
    AVStreamInfo video_stream_info = {};

    ReadVideoMetaData(callParams.input_file, info);

    uint8_t *pixel_data = static_cast<uint8_t *>(
        malloc(info.width_resolution * info.height_resolution * 4));
    if (!pixel_data)
    {
        std::cout << "Failed to allocate framebuffer!\n";
        exit(1);
    }

    *callParams.trim_end = info.duration;

    double duration_seconds =
        info.duration * (double)info.VideoTimeBase.num / (double)info.VideoTimeBase.den;
    std::string human_time_duration = GetHumanTimeString(duration_seconds);

    GLFWwindow *window = InitGL();

    GUI::Initialize(window);
    GUI::SetStyles();

    if (!OpenVideoStream(callParams.input_file, video_stream_info))
    {
        LOGD("Big problemo");
        CloseVideoStream(video_stream_info);
        exit(1);
    }
    if (!OpenAudioStream(callParams.input_file, video_stream_info))
    {
        LOGD("No Audio stream");
        CloseAudioStream(video_stream_info);
        has_audio_stream = false;
    }

    GLuint hTex;
    glGenTextures(1, &hTex);
    glBindTexture(GL_TEXTURE_2D, hTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    unsigned int shader = graphics::CreateShaderProgram();

    *callParams.trim_start = 0;
    GUI::GUIState guiState = {};

    // Read first frame
    if (!ReadFrameSeek(*callParams.trim_start, video_stream_info,
                       pixel_data, guiState.precision_seek))
    {
        LOGD("Failed to Read first frame!");
        exit(1);
    }
    // Read audio
    int16_t *audio_data = nullptr;
    if (video_stream_info.AudioCodecContext)
    {
        int channels = video_stream_info.AudioCodecContext->ch_layout.nb_channels;
        int sample_rate = video_stream_info.AudioCodecContext->sample_rate;
        int buffer_size = 10 * sample_rate * channels;
        audio_data = new int16_t[buffer_size];
    }
    // move to AL header
    const int BUFFER_COUNT = 4;
    ALuint buffers[BUFFER_COUNT];
    ALuint source;
    alGenBuffers(BUFFER_COUNT, buffers);
    alGenSources(1, &source);

    // reading initial data
    int samples_read = 0;

    static double last_video_frame_time = 0;
    static double time_for_10_frames = 0;

    while (!glfwWindowShouldClose(window))
    {
        int windowW, windowH;
        glfwGetWindowSize(window, &windowW, &windowH);

        GUI::CreateInterface(guiState, callParams, info);

        if (guiState.export_called)
        {
            Export(callParams, guiState);
        }

        switch (guiState.sliderState)
        {
        case GUI::SliderState::FIRST_VALUE_CHANGED:
        {
            if (!ReadFrameSeek(*callParams.trim_start, video_stream_info,
                               pixel_data, guiState.precision_seek))
            {
                LOGD("Failed to read frame with new method\n");
                CloseVideoStream(video_stream_info);
                exit(1);
            }
            OpenAL::PauseAudioSamples(buffers,source,BUFFER_COUNT);

            Playback=false;
            first_playback_frame=true;
            break;
        }
        case GUI::SliderState::SECOND_VALUE_CHANGED:
        {
            if (!ReadFrameSeek(*callParams.trim_end, video_stream_info,
                               pixel_data, guiState.precision_seek))
            {
                LOGD("Failed to read frame with new method\n");
                CloseVideoStream(video_stream_info);
                exit(1);
            }
            OpenAL::PauseAudioSamples(buffers,source,BUFFER_COUNT);
            Playback=false;
            first_playback_frame=true;

            break;
        }
        default:
            break;
        }
        static uint32_t timestamp;
        if (guiState.play_pushed)
        {
            Playback = !Playback;
            guiState.play_pushed = false;
            if (Playback)
                first_playback_frame = true;
            glfwSetTime(0.0);
        }

        if (Playback)
        {
            double video_native_fps = info.frame_rate.num / info.frame_rate.den;

            //TODO: Make sure this works actually not sure if this works...
            auto start_time = *callParams.trim_start;
            auto end_time = *callParams.trim_end;

            if (!has_audio_stream)
            {
                timestamp = start_time + ToPTS(glfwGetTime(),info.VideoTimeBase);
                bool got_frame = false;
                //TODO: Add the fix from below. Get the assumed next frame time, and only push a frame when glfwGetTime overshoots that frame
                if (timestamp < end_time)
                    ReadNextVideoFrame(timestamp, video_stream_info, pixel_data);
            }

            else
            {

                if (first_playback_frame)
                {
                    //Get audio pts of video_pts
                    double audio_num = info.AudioTimeBase.num;
                    double audio_den = info.AudioTimeBase.den;
                    double video_seconds=ToSeconds(start_time,info.VideoTimeBase);
                    uint32_t audio_pts=ToPTS(video_seconds,info.AudioTimeBase);
                    double audio_seconds=ToSeconds(audio_pts,info.AudioTimeBase);

                    samples_read = ReadInitialAudioFrame(audio_pts, video_stream_info, audio_data);
                    if(samples_read<=0)
                    {
                        LOGD("Failed to read audio samples in playback");
                        exit(1);
                    }
                    timestamp = ToPTS(audio_seconds,info.VideoTimeBase);
                    if (!ReadFrameSeek(timestamp, video_stream_info, pixel_data, true))
                            {
                                LOGD("Failed initial seek on playback");
                            } 
                    first_playback_frame=false;
                    printf("Reading audio at %f s and video at %f s\n",audio_seconds,video_seconds);
                   
                }

                if (timestamp < end_time)
                {
                    //Play audio first, and sync video to it. 
                    //This function modifies the current_audio_pts, which we use to sync
                    OpenAL::PlayAudioSamples(video_stream_info, samples_read, audio_data, BUFFER_COUNT, buffers, source,video_behind);

                    double video_frame_output_time = GetFPS(last_video_frame_time,video_native_fps);
                   
                    double audio_seconds = ToSeconds(OpenAL::audio_current_pts,info.AudioTimeBase);

                    timestamp =  ToPTS(audio_seconds,info.VideoTimeBase);

                    // Calculate the pts of the next frame, and get that timestamp.
                    uint32_t frame_time_pt = ToPTS(1,info.frame_rate);
                    double next_frame_seconds = ToSeconds(video_stream_info.last_video_pts+frame_time_pt,info.VideoTimeBase);
                    double timestamp_seconds = ToSeconds(timestamp,info.VideoTimeBase);

                    // Use that timestamp and sync to audio.
                    auto slip = abs(next_frame_seconds - timestamp_seconds);
                    auto slip_pts = ToPTS(slip,info.VideoTimeBase); 
                    double acceptable_slip = 0.04;
                    
                    // If the audio and video timestamps are within 0.04 seconds of eachother, read the frame
                    if (slip < acceptable_slip)
                    {
                        ReadNextVideoFrame(timestamp, video_stream_info, pixel_data);

                        last_video_frame_time = glfwGetTime();

                        if (frame_accumulator < ACCUMULATOR_SAMPLE_SIZE)
                        {
                            frametime_accumulator += video_frame_output_time;
                            frame_accumulator++;
                        }
                    }

                    else
                    {
                        //  If the next video_frame is ahead of the next audio frame, we skip this iteration,
                        //  and the audio will catch up on next run
                        if (next_frame_seconds > timestamp_seconds)
                        {
                            video_behind = false;
                        }

                        // If the audio is ahead of video, beyond the acceptable tolerance,
                        // Halt the audio with the video_behind bool, and do a hard seek to the next video frame with
                        // the correct timestamp
                        else
                        {
                            printf("Audio ahead of video (a): %f ,(v) %f \n", timestamp_seconds, next_frame_seconds);
                            // Pause audio for the difference
                            video_behind = true;
                            if (!ReadFrameSeek(timestamp + slip_pts, video_stream_info, pixel_data, true))
                            {
                                printf("Failed seeking to %f\n", audio_seconds);
                            }
                            printf("Seeking video to %f\n", ToSeconds((double)video_stream_info.last_video_pts,info.VideoTimeBase)); 
                        }
                    }
                    guiState.video_audio_time_difference = slip;
                }

                else
                {
                    // Reset playback
                    glfwSetTime(0.0);
                }
                guiState.playback_timestamp_seconds = ToSeconds(timestamp,info.VideoTimeBase);
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width_resolution,
                     info.height_resolution, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixel_data);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        dock_height = guiState.dock_height;
        auto render_window_height = windowH;
        auto render_window_width = windowW;

        if (renderFirstFrame)
        {

            float xscale, yscale;
            glfwGetWindowContentScale(window, &xscale, &yscale);
            int scaled_width = render_window_width * xscale;
            int scaled_height = render_window_height * yscale;
            CreateVideoSurface(info, scaled_width, scaled_height, xscale, yscale);
            renderFirstFrame = false;
        }
        DrawVideoFrame(hTex, shader);
        GUI::Present();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    CloseVideoStream(video_stream_info);
    OpenAL::CloseAudioStream(audio_device, audio_context);
}

GLFWwindow *InitGL()
{
    GLFWwindow *window;
    GLuint shaderProgram, computeProgram, VBO, VAO, EBO, heightMapTexture,
        vertexSSBO;

    glfwInit();
    glfwSetErrorCallback(graphics::glfwErrorCallback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    window = glfwCreateWindow(1280, 720, "Trim", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "PROBLEM GL";
        return nullptr;
    }
    glfwSetWindowSizeCallback(window,
                              [](GLFWwindow *window, int width, int height)
                              {
                                  float xscale, yscale;
                                  glfwGetWindowContentScale(window, &xscale, &yscale);
                                  int scaled_width = width * xscale;
                                  int scaled_height = height * yscale;

                                  glViewport(0, 0, scaled_width, scaled_height);
                                  HandleVideoFrameResize(info, scaled_width, scaled_height, xscale, yscale);
                              });

    return window;
}

void HandleVideoFrameResize(const video_info &info, int display_width, int display_height, int xscale, int yscale)
{
    CreateVideoSurface(info, display_width, display_height, xscale, yscale);
}

// Create and Assign VAO for video frame
//  ( Called on init, and resize )
void CreateVideoSurface(const video_info &info, int display_width, int display_height, int xscale, int yscale)
{

    int video_dock_gap = 100;
    auto usable_height = display_height - (dock_height * yscale + video_dock_gap);

    float minX, minY, maxX, maxY;
    float aspectRatio = (float)info.width_resolution / (float)info.height_resolution;
    minY = 0, maxY = usable_height;
    float frame_width = maxY * aspectRatio;
    minX = ((float)display_width - (float)frame_width) / 2;
    maxX = minX + frame_width;

    float verts[] = {
        minX, minY, 0.0f, 0.0f, // x,y,u,v
        maxX, minY, 1.0f, 0.0f,
        maxX, maxY, 1.0f, 1.0f,
        minX, minY, 0.0f, 0.0f,
        maxX, maxY, 1.0f, 1.0f,
        minX, maxY, 0.0f, 1.0f};
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    proj = glm::ortho(0.0f, (float)display_width, (float)display_height, 0.0f, -1.0f, 1.0f);
}

void DrawVideoFrame(GLuint tex, unsigned int shader_program)
{
    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1,
                       GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE,
                       glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SendSystemCommand(const char *command)
{
    int result = std::system(command);

    if (result == 0)
    {
        std::cout << "Video trimmed successfully: " << std::endl;
    }
    else
    {
        std::cerr << "Error: ffmpeg command failed with code " << result
                  << std::endl;
    }
    std::cout << "The command entered was :\n\n"
              << command << "\n";
}

void Export(SystemCallParameters &callParams, GUI::GUIState &guiState)
{

    if (!guiState.output_string_built)
    {
        fs::path inPath = (callParams.input_file);
        fs::path inputFileDirectory = inPath.parent_path();
        fs::path outPath = callParams.output_file;

        if (callParams.custom_output_file)
        {
            fs::path strippedFilename = outPath.filename();

            if (outPath == strippedFilename)
            {
                LOGD("Output file did not contain a path location::Output file will "
                     "be "
                     "placed in input directory");
                LOGD("Input directory: " << fs::absolute(inputFileDirectory.string()));
                outPath = inputFileDirectory / strippedFilename;
                LOGD("Full output path will be :" << fs::absolute(outPath).string());
            }
            else
            {
                LOGD("An output location was given\n");
            }
        }

        FFMPEG_OUTPUT_COMMAND =
            "ffmpeg -accurate_seek -i  \"" +
            fs::absolute(callParams.input_file).string() + "\" -ss " +
            GetHumanTimeString(callParams.startTimeSeconds) + " -to " +
            GetHumanTimeString(callParams.endTimeSeconds) + "  \"" +
            fs::absolute(outPath).string() + "\"";

        std::strncpy(callParams.output_buffer, FFMPEG_OUTPUT_COMMAND.c_str(),
                     sizeof(callParams.output_buffer) - 1);
        guiState.output_string_built = true;
    }
    if (guiState.display_editor_popup)
        return;

    SendSystemCommand(callParams.output_buffer);
    exit(1);
}

void ProcessArgs(int argc, char *argv[], SystemCallParameters &callParams)
{

    if (argc < 2 || argc > 3)
    {
        PrintUsageError();
        exit(1);
    }
    std::strncpy(callParams.input_file, argv[1], MAX_PATH_BYTES - 1);
    callParams.input_file[MAX_PATH_BYTES - 1] = '\0';

    if (argc > 2)
    {
        std::strncpy(callParams.output_file, argv[2], MAX_PATH_BYTES - 1);
        callParams.output_file[MAX_PATH_BYTES - 1] = '\0';
        callParams.custom_output_file = true;
    }
    else
    {
        if (!ProcessFilename(callParams.input_file, callParams.output_file))
            exit(1);
        std::cout << "Trimmed video will be written to " << callParams.output_file
                  << "\n";
    }
}
