#define _CRT_SECURE_NO_WARNINGS
#include <AL/al.h>
#include <AL/alc.h>
#include <mutex>
#include <queue>
#include "trim.h"
namespace fs = std::filesystem;
// [x]TODO: Assign C++ 17 as the standard.
// [x]TODO: Make slider animation independent of framerate
// [x]TODO:PRIORITY: Fast seeking. We should not be closing and opening the input
// [x]TODO: GUI: We need the dock to be already docked in when the program is
// launched.
// [ ]TODO: Fix our resizing functionality. Currently, the resize only works half the time, and sometimes the render frame is not sized correctly upon starting the program.
//
// [ ]TODO: App: We're probably going to need playback for this to be viable for
// actual editing.
// for each frame. This makes no sense. Whatever overhead this actually costs,
// we don't want to incur this.
// [ ]TODO: Clean up the cmake shit for SURE.

// [ ]TODO: Remove all these static variables probably

//[ ]TODO: Initialize the software context exactly once, not per frame.
//[ ]TODO: If all else fails, create an audio thread with a shared queue. 
//[ ]TODO: Make seperate video, and audio pFormat contexts. ie: break apart any similarities in the audio and video reads. They will be their own threads




static int render_window_height;
static int render_window_width;
static float minX, minY, maxX, maxY;
static float aspectRatio;
static float frame_width;
static glm::mat4 model(1.0f);

static int windowW, windowH;

static std::string FFMPEG_OUTPUT_COMMAND;
GLuint VAO, VBO;
glm::mat4 proj;

bool renderFirstFrame = true;
static bool first_run = true;
static int64_t audio_current_pts = 0;
static bool Playback = false;

video_info info{};
#define ALSOFT_LOGLEVEL 3
const int ACCUMULATOR_SAMPLE_SIZE = 30;
static double frametime_accumulator = 0;
static int frame_accumulator = 0;

static bool running = false;

bool SetupAudio(ALCdevice* device, ALCcontext* context)
{

    device = alcOpenDevice(nullptr);
    if (!device)
    {
        LOGD("Failed to open audio device");
        return false;
    }
    context = alcCreateContext(device, nullptr);
    if (!context)
    {
        alcCloseDevice(device);
        LOGD("Failed to create audio context");
        return false;
    }
    alcMakeContextCurrent(context);
    LOGD("Initialized OpenAL context");

    return true;
}
void PlayAudioSamples(AVStreamInfo& video_stream_info, int num_samples, int16_t* audio_data, const int BUFFER_COUNT, ALuint* buffers, ALuint source)
{

    //TODO: Get actual audio num and den
    //TODO: Make all of this actually obey the trim



    auto channels = video_stream_info.AudioCodecContext->ch_layout.nb_channels;
    auto sample_rate = video_stream_info.AudioCodecContext->sample_rate;

    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;


    if (first_run)
    {
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        audio_current_pts = video_stream_info.last_audio_pts;

        // Fill first buffer with provided audio_data
        alBufferData(buffers[0], format, audio_data, num_samples * sizeof(int16_t), sample_rate);

        // Fill the other 3 buffers

        for (int i = 1; i < BUFFER_COUNT; i++)
        {

            int16_t* data = ReadNextAudioFrame(video_stream_info);
            alBufferData(buffers[i], format, data,
                num_samples * sizeof(int16_t), sample_rate);
            free(data);
        }

        alSourceQueueBuffers(source, BUFFER_COUNT, buffers);
        alSourcePlay(source);
        first_run = false;
        ALint queued;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    }
    else
    {
        ALint queued, processed;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);


        while (processed > 0)
        {
            ALuint buffer;
            // Unqueue a processed buffer
            alSourceUnqueueBuffers(source, 1, &buffer);
            audio_current_pts = video_stream_info.last_audio_pts;
            // Fill it with new data



            int16_t* new_data = ReadNextAudioFrame(video_stream_info);

            alBufferData(buffer, format, new_data,
                num_samples * sizeof(int16_t), sample_rate);
            delete new_data;

            // Queue it back
            alSourceQueueBuffers(source, 1, &buffer);

            processed--;
        }

        // Check if playback has stopped due to underrun
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            alSourcePlay(source);
        }
    }

    // Check for OpenAL errors
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        printf("OpenAL error: %d\n", alGetString(error));
        return;
    }


}

void CloseAudioStream(ALCdevice* device, ALCcontext* context)
{
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

int main(int argc, char* argv[])
{
    SystemCallParameters callParams = {};
    callParams.trim_start = new uint32_t(1);
    callParams.trim_end = new uint32_t(1);
    ProcessArgs(argc, argv, callParams);
    ALCdevice* audio_device = nullptr;
    ALCcontext* audio_context = nullptr;
    SetupAudio(audio_device, audio_context);
    AVStreamInfo video_stream_info = {};

    ReadVideoMetaData(callParams.input_file, info);

    uint8_t* pixel_data = static_cast<uint8_t*>(
        malloc(info.width_resolution * info.height_resolution * 4));
    if (!pixel_data)
    {
        std::cout << "Failed to allocate framebuffer!\n";
        exit(1);
    }

    *callParams.trim_end = info.duration;

    // Maybe for displaying the total duration of the trimmed video? IDK where i'm
    // using this
    double duration_seconds =
        info.duration * (double)info.VideoTimeBase.num / (double)info.VideoTimeBase.den;
    std::string human_time_duration = GetHumanTimeString(duration_seconds);

    GLFWwindow* window = InitGL();

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
    }

    GLuint hTex;
    glGenTextures(1, &hTex);
    glBindTexture(GL_TEXTURE_2D, hTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    unsigned int shader = CreateShaderProgram();

    glfwGetWindowSize(window, &windowW, &windowH);

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
    int16_t* audio_data = nullptr;
    if (video_stream_info.AudioCodecContext)
    {
        int channels = video_stream_info.AudioCodecContext->ch_layout.nb_channels;
        int sample_rate = video_stream_info.AudioCodecContext->sample_rate;
        int buffer_size = 10 * sample_rate * channels;
        audio_data = new int16_t[buffer_size];
    }


    const int BUFFER_COUNT = 4;
    ALuint buffers[BUFFER_COUNT];
    ALuint source;
    alGenBuffers(BUFFER_COUNT, buffers);
    alGenSources(1, &source);
    int samples_read = ReadInitialAudioFrame(0, video_stream_info, audio_data);
    static double last_video_frame_time = 0;
    static double time_for_10_frames = 0;






    while (!glfwWindowShouldClose(window))
    {
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

            break;
        }
        default:
            break;
        }
        static uint32_t timestamp;
        static uint32_t timestamp_seconds;
        if (guiState.play_pushed)
        {
            Playback = !Playback;
            guiState.play_pushed = false;
            glfwSetTime(0.0);
        }

        //How this MIGHT need to work
/*
    On Playback pushed, we get the initial frame for audio and video
    We grab the very next frame in a loop
    TODO: Video is RUSHING audio.


*/




        if (Playback)
        {
            double video_native_fps = info.frame_rate.num / info.frame_rate.den;
            double toSeconds = ((double)info.VideoTimeBase.num / (double)info.VideoTimeBase.den);

            double toPTS =
                ((double)info.VideoTimeBase.den / (double)info.VideoTimeBase.num);

            if (timestamp < *callParams.trim_end)
            {
                if (samples_read > 0)
                {
                    PlayAudioSamples(video_stream_info, samples_read, audio_data, BUFFER_COUNT, buffers, source);
                }
                else
                {
                    LOGD("Didn't read any audio samples");
                }
                auto frame_start = glfwGetTime();
                auto video_frame_output_time = frame_start - last_video_frame_time;


                if (frame_accumulator == ACCUMULATOR_SAMPLE_SIZE)
                {
                    double ms = (double)frametime_accumulator / ACCUMULATOR_SAMPLE_SIZE;
                    int FPS = floor(((double)ACCUMULATOR_SAMPLE_SIZE / (double)frametime_accumulator));
                    //printf("OUTPUT: FPS=>%d  | Native: %d\n", FPS, (int)video_native_fps);
                    frame_accumulator = 0;
                    frametime_accumulator = 0;

                }




                double audio_num = info.AudioTimeBase.num;
                double audio_den = info.AudioTimeBase.den;
                double audio_seconds = audio_current_pts * (audio_num /audio_den);
                //double audio_seconds = ((double)audio_current_pts * audio_num) / (double)audio_den;
                timestamp =
                    *callParams.trim_start +
                    audio_seconds * toPTS;

                double frame_time_seconds = (double)info.frame_rate.den / (double)info.frame_rate.num;
                auto frame_time_pt = (frame_time_seconds * toPTS);
                auto next_frame_pts = video_stream_info.last_video_pts + frame_time_pt;
                auto next_frame_seconds = next_frame_pts * toSeconds;
                auto timestamp_seconds = timestamp * toSeconds;

                //printf("\n________________\nNextFrameSeconds:%f\nTimeStampSeconds:%f\n________________\n ", next_frame_seconds, timestamp_seconds);
                auto slip = abs(next_frame_seconds - timestamp_seconds);
                auto slip_pts = slip * toPTS;
                printf("SLIP:%f\n", slip);

                double wiggleroom = 0.03;
                int wiggleroom_pts = wiggleroom * toPTS;
                bool got_frame = false;
                double acceptable_timestamp = next_frame_seconds + wiggleroom;
                if (slip < 0.04)
                {
                    //When called from here, this function should never present a frame.
                   //It will increment until it is greater than.
                    ReadNextVideoFrame(timestamp, wiggleroom_pts, got_frame, video_stream_info, pixel_data);

                    last_video_frame_time = glfwGetTime();

                    if (frame_accumulator < ACCUMULATOR_SAMPLE_SIZE)
                    {
                        frametime_accumulator += video_frame_output_time;
                        frame_accumulator++;
                    }
                }
                else
                {
                    printf("Discarding frame\n");
                }
                //else if (next_frame_seconds+wiggleroom>timestamp_seconds){
                //    //When called from here, this function should never present a frame.
                ////It will increment until it is greater than.
                //    ReadNextVideoFrame(timestamp, wiggleroom_pts, got_frame, video_stream_info, pixel_data);

                //    last_video_frame_time = glfwGetTime();

                //    if (frame_accumulator < ACCUMULATOR_SAMPLE_SIZE)
                //    {
                //        frametime_accumulator += video_frame_output_time;
                //        frame_accumulator++;

                //    }
                //}
                
                //if(acceptable_timestamp<timestamp_seconds)
                //{
                //   
                //}
                //else if(acceptable_timestamp>timestamp_seconds)
                //{
                //    //printf("Frames are ahead %f\n", slip);
                //    if(slip<wiggleroom)
                //    {
                //        //printf("Frames are ahead\n");
                //        ReadNextVideoFrame(timestamp,wiggleroom_pts,got_frame, video_stream_info, pixel_data);

                //        last_video_frame_time = glfwGetTime();

                //        if (frame_accumulator < ACCUMULATOR_SAMPLE_SIZE)
                //        {
                //            frametime_accumulator += video_frame_output_time;
                //            frame_accumulator++;
                //        }
                //    }
                //    else{
                //        //printf("Discarding frame\n");
                //    }
                //}



            }

            else
            {
                glfwSetTime(0.0);
            }

            guiState.playback_timestamp_seconds = timestamp * toSeconds;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width_resolution,
            info.height_resolution, 0, GL_RGBA, GL_UNSIGNED_BYTE,
            pixel_data);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwGetWindowSize(window, &windowW, &windowH);

        render_window_height = windowH - guiState.dock_height;
        render_window_width = windowW;

        proj = glm::ortho(0.0f, (float)windowW, (float)windowH, 0.0f, -1.0f, 1.0f);
        if (renderFirstFrame)
        {
            CreateVideoSurface(info);
            renderFirstFrame = false;
        }
        DrawVideoFrame(hTex, shader);
        GUI::Present();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    CloseVideoStream(video_stream_info);
    CloseAudioStream(audio_device, audio_context);
}

void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

GLFWwindow* InitGL()
{
    GLFWwindow* window;
    GLuint shaderProgram, computeProgram, VBO, VAO, EBO, heightMapTexture,
        vertexSSBO;

    glfwInit();
    glfwSetErrorCallback(glfwErrorCallback);
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
    glfwSetFramebufferSizeCallback(window,
        [](GLFWwindow* window, int width, int height)
        {
            glViewport(0, 0, width, height);
            HandleVideoFrameResize(info);
        });

    return window;
}

bool ProcessFilename(const char* inFile, char* outFile)
{

    const char* dot = strrchr(inFile, '.');
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

void PrintUsageError()
{
    std::cerr << "Usage: trim <input_file> [output_file]\n"
        "  input_file:  Required. The file to process ( must include "
        "extension)\n"
        "  output_file: Optional. The destination file (if omitted, "
        "will be auto-generated)\n";
}

void HandleVideoFrameResize(const video_info& info)
{
    CreateVideoSurface(info);
}

unsigned int CreateShaderProgram()
{

    const char* vertShader =
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
        "    TexCoord = aTexCoord;\n"
        "}\n";

    const char* fragShader = "#version 330 core\n"
        "in vec2 TexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D tex;\n"
        "void main() {\n"
        "    FragColor = texture(tex, TexCoord);\n"
        "}\n";

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertShader, NULL);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragShader, NULL);
    glCompileShader(frag);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return shader;
}
// Create and Assign VAO for video frame
//  ( Called on init, and resize )
void CreateVideoSurface(const video_info& info)
{
    aspectRatio = (float)info.width_resolution / (float)info.height_resolution;
    minY = 0, maxY = render_window_height;
    frame_width = maxY * aspectRatio;
    minX = (render_window_width - frame_width) / 2;
    maxX = minX + frame_width;

    float verts[] = { minX, minY, 0.0f, 0.0f, // x,y,u,v
                     maxX, minY, 1.0f, 0.0f, maxX, maxY, 1.0f, 1.0f, minX, minY,
                     0.0f, 0.0f, maxX, maxY, 1.0f, 1.0f, minX, maxY, 0.0f, 1.0f };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
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

void SendSystemCommand(const char* command)
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

void Export(SystemCallParameters& callParams, GUI::GUIState& guiState)
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

void ProcessArgs(int argc, char* argv[], SystemCallParameters& callParams)
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
