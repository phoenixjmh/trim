#define _CRT_SECURE_NO_WARNINGS
#include "trim.h"
namespace fs = std::filesystem;
//TODO: Assign C++ 17 as the standard.
//TODO: GUI: We need the dock to be already docked in when the program is launched. 
//TODO: App: We're probably going to need playback for this to be viable for actual editing.
//TODO:PRIORITY: Fast seeking. We should not be closing and opening the input for each frame. This makes no sense. Whatever overhead this actually costs, we don't want to incur this.

static int render_window_height;
static int render_window_width;
static  float minX, minY, maxX, maxY;
static float aspectRatio;
static float frame_width;
static glm::mat4 model(1.0f);

static int windowW, windowH;

static std::string FFMPEG_OUTPUT_COMMAND;
GLuint VAO, VBO;
glm::mat4 proj;

bool renderFirstFrame = true;

video_info info{};

int main(int argc, char* argv[])
{
    SystemCallParameters callParams = {};
    callParams.trim_start = new uint32_t(1);
    callParams.trim_end = new uint32_t(1);
    ProcessArgs(argc, argv, callParams);


    

    ReadVideoMetaData(callParams.input_file, info);


    uint8_t* pixel_data = static_cast<uint8_t*>(malloc(info.width_resolution * info.height_resolution * 4));
    if (!pixel_data)
    {
        std::cout << "Failed to allocate framebuffer!\n";
        exit(1);
    }





    *callParams.trim_end = info.duration;

    //Maybe for displaying the total duration of the trimmed video? IDK where i'm using this
    double duration_seconds = info.duration * (double)info.time_base.num / (double)info.time_base.den;
    std::string human_time_duration = GUI::GetHumanTimeString(duration_seconds);

    GLFWwindow* window = InitGL();

    GUI::Initialize(window);
    GUI::SetStyles();

    //Read first frame
    if (!ReadFrame(callParams.input_file, pixel_data, *callParams.trim_start))
    {
        std::cout << "Error::Failed to read first frame\n";
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




    GUI::GUIState guiState = {};



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

            if (!ReadFrame(callParams.input_file, pixel_data, *callParams.trim_start))
            {
                exit(1);
            }
            break;
        case GUI::SliderState::SECOND_VALUE_CHANGED:

            if (!ReadFrame(callParams.input_file, pixel_data, *callParams.trim_end))
            {
                exit(1);
            }
            break;
        default:
            break;
        }



        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width_resolution, info.height_resolution, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, pixel_data);

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
}

void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

GLFWwindow* InitGL()
{
    GLFWwindow* window;
    GLuint shaderProgram, computeProgram, VBO, VAO, EBO, heightMapTexture, vertexSSBO;

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
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
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
        "  input_file:  Required. The file to process ( must include extension)\n"
        "  output_file: Optional. The destination file (if omitted, will be auto-generated)\n";
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

    const char* fragShader =
        "#version 330 core\n"
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
//Create and Assign VAO for video frame
// ( Called on init, and resize )
void CreateVideoSurface(const video_info& info)
{
    aspectRatio = (float)info.width_resolution / (float)info.height_resolution;
    minY = 0, maxY = render_window_height;
    frame_width = maxY * aspectRatio;
    minX = (render_window_width - frame_width) / 2;
    maxX = minX + frame_width;

    float verts[] =
    {
        minX, minY, 0.0f, 0.0f,  // x,y,u,v
        maxX, minY, 1.0f, 0.0f,
        maxX, maxY, 1.0f, 1.0f,
        minX, minY, 0.0f, 0.0f,
        maxX, maxY, 1.0f, 1.0f,
        minX, maxY, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

}

void DrawVideoFrame(GLuint tex, unsigned int shader_program)
{
    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));
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
        std::cerr << "Error: ffmpeg command failed with code " << result << std::endl;
    }
    std::cout << "The command entered was :\n\n" << command << "\n";

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
                LOGD( "Output file did not contain a path location::Output file will be placed in input directory");
                LOGD("Input directory: " << fs::absolute(inputFileDirectory.string()));
                outPath = inputFileDirectory / strippedFilename;
                LOGD("Full output path will be :" << fs::absolute(outPath).string());
                
            }
            else
            {
                LOGD("An output location was given\n");
                
            }

        }



        FFMPEG_OUTPUT_COMMAND = "ffmpeg -accurate_seek -i  \"" + fs::absolute(callParams.input_file).string() +
            "\" -ss " + GUI::GetHumanTimeString(callParams.startTimeSeconds) +
            " -to " + GUI::GetHumanTimeString(callParams.endTimeSeconds) +
            "  \"" + fs::absolute(outPath).string() + "\"";

        std::strncpy(callParams.output_buffer, FFMPEG_OUTPUT_COMMAND.c_str(), sizeof(callParams.output_buffer) - 1);
        guiState.output_string_built = true;
    }
    if (guiState.display_editor_popup)
        return;

    SendSystemCommand(callParams.output_buffer);
    exit(1);
}

void ProcessArgs(int argc, char* argv[], SystemCallParameters& callParams)
{

    if (argc < 2 || argc>3)
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
        //TODO make this more specific for sure...
        std::cout << "Trimmed video will be written to " << callParams.output_file << "\n";
    }
}






