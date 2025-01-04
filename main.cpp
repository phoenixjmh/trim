#define _CRT_SECURE_NO_WARNINGS
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "video_operations.h"
#include <string>
#include <sstream>
#include <cstdio>
#include <cmath>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gui.h"
#include "range_slider.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <cassert>
extern "C" {
#include <stdio.h>
    //#include <unistd.h>
};

static int render_window_height;
static int render_window_width;
static  float minX, minY, maxX, maxY;
static float aspectRatio;
static float frame_width;
static glm::mat4 model(1.0f);

static int windowW, windowH;

static std::string FFMPEG_OUTPUT_COMMAND;
static char output_buffer[1024];

bool renderFirstFrame = true;



struct GUIState
{
    float dock_height;
    float dock_width;
    bool export_called = false;
    bool output_string_built = false;
    bool preview_output_command = false;
    ImGui::RangeSliderChangeType trimSliderState;

};

struct SystemCallParameters
{
    uint32_t* trim_start;
    uint32_t* trim_end;
    char* input_file;
    char* output_file;
};


video_info info{};


void CreateVideoSurface(const video_info& info);
void HandleVideoFrameResize(const video_info& info);

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

std::string GetHumanTimeString(uint32_t time_seconds)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d:%.2f", (int)floor((double)time_seconds / 60.0), std::floor(((((double)time_seconds / 60.0) - (floor(((double)time_seconds / 60.0)))) * 60) * 100.0) / 100.0);
    return{ buffer };
}

int processFileName(const char* inFile, char* outFile)
{
    const char* dot = strrchr(inFile, '.');
    if (!dot)
    {
        std::cerr << "Input file must include extension\n";
        return 1;
    }

    size_t nameLen = (size_t)(dot - inFile);
    memcpy(outFile, inFile, nameLen);
    memcpy(outFile + nameLen, "_trimmed", 8);
    strcpy(outFile + nameLen + 8, dot);
    return 0;

}

void PrintUsageError()
{
    std::cerr << "Usage: trim <input_file> [output_file]\n"
        "  input_file:  Required. The file to process ( must include extension)\n"
        "  output_file: Optional. The destination file (if omitted, will be auto-generated)\n";
}

std::string CreateOutputFileName(const std::string& inFile)
{
    size_t dot_pos = inFile.rfind('.');
    std::string newName = inFile.substr(0, dot_pos) + "_trimmed" + inFile.substr(dot_pos);
    return newName;
}

void HandleVideoFrameResize(const video_info& info)
{
    CreateVideoSurface(info);
}
// === GL HELPERS ===
GLuint VAO, VBO;
glm::mat4 proj;


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

void DisplayCommandEditor(char* output_buffer, GUIState& state)
{
    ImGui::OpenPopup("Edit Command");
    if (ImGui::BeginPopupModal("Edit Command", nullptr, ImGuiWindowFlags_None))
    {
       
        ImGui::InputTextMultiline("Final command", output_buffer, 1024, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16));
        ImGui::Separator();

        if (ImGui::Button("Finalize & Export", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            state.preview_output_command = false;
        }

        ImGui::EndPopup();
    }
}



void Export(const char* infile, const char* outfile,double start,double end,GUIState& state)
{
    if (!state.output_string_built)
    {

        FFMPEG_OUTPUT_COMMAND = "ffmpeg -accurate_seek -i  \"" + std::string(infile) +
            "\" -ss " + GetHumanTimeString(start) +
            " -to " + GetHumanTimeString(end) +
            "  \"" + std::string(outfile) + "\"";

        std::strncpy(output_buffer, FFMPEG_OUTPUT_COMMAND.c_str(), sizeof(output_buffer) - 1);
        state.output_string_built= true;
    }
    if (state.preview_output_command)
    {
        DisplayCommandEditor(output_buffer,state);
    }
    else
    {
        SendSystemCommand(output_buffer);
        exit(1);
    }
}



void CreateGuiInterface(GUIState& guiState,SystemCallParameters& callParams,const video_info& info)
{

    auto startTimeSeconds = *callParams.trim_start * (double)info.time_base.num / (double)info.time_base.den;
    auto endTimeSeconds = *callParams.trim_end* (double)info.time_base.num / (double)info.time_base.den;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

    ImGui::Begin("Video Trimmer", nullptr, ImGuiWindowFlags_NoDecoration);


    guiState.dock_height = ImGui::GetContentRegionAvail().y;
    guiState.dock_width = ImGui::GetContentRegionAvail().x;

    ImGui::Text("Trim Range Selection");
    ImGui::Separator();


    float fullWidth = ImGui::GetContentRegionAvail().x;


    ImGui::PushItemWidth(fullWidth);
    ImGui::RangeSliderChangeType change_type =
        ImGui::RangeSliderUInt("##TimeRange", callParams.trim_start, callParams.trim_end, 0, info.duration, "%.2f", 1);

    ImGui::PopItemWidth();
    ImGui::Text("Start Time: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", GetHumanTimeString(startTimeSeconds).c_str());

    ImGui::SameLine(200);
    ImGui::Text("End Time: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", GetHumanTimeString(endTimeSeconds).c_str());

    
    guiState.trimSliderState = change_type;
   

    ImGui::Separator();
    ImGui::Checkbox("Preview output before export", &guiState.preview_output_command);
    if (ImGui::Button("Export Video"))
    {
        guiState.export_called = true;
    }

    if(guiState.export_called)
    {
        Export(callParams.input_file, callParams.output_file, startTimeSeconds, endTimeSeconds,guiState);
    }
   

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Adjust the sliders and click Export to save the trimmed video.");
    ImGui::End();
    ImGui::Render();

}



struct args
{
    char* input_file;
    char output_file[256];
};

bool ProcessArgs(int argc, char* argv[], args* outArgs)
{
    //TODO: if desired output_name already exists, the system freezes...
    if (argc < 2 || argc>3)
    {
        PrintUsageError();
        return false;
    }
    outArgs->input_file = argv[1];

    if (argc > 2)
        strcpy(outArgs->output_file, argv[2]);
    else
    {
        int response = processFileName(outArgs->input_file, outArgs->output_file);
        if (response != 0)
            return false;
        std::cout << "Trimmed video will be written to " << outArgs->output_file << "\n";
    }
    return true;
}
int main(int argc, char* argv[])
{
    args outArgs{};
    if (!ProcessArgs(argc, argv, &outArgs))
        return 1;

    auto input_file = outArgs.input_file;
    auto output_file = outArgs.output_file;



    GLFWwindow* window = InitGL();


    InitializeGUI(window);
    SetImGuiStyles();



    if (!ReadVideoMetaData(outArgs.input_file, info))
    {
        return -1;
    }

    uint8_t* pixel_data = static_cast<uint8_t*>(malloc(info.width_resolution * info.height_resolution * 4));
    assert(pixel_data && "Couldn't allocate framebuffer\n");
    
    SystemCallParameters callParams = {};


  
   
    callParams.trim_start= new uint32_t(1);
    callParams.trim_end = new uint32_t(1);
    callParams.input_file = input_file;
    callParams.output_file = output_file;

    //Read first frame
    if (!ReadFrame(callParams.input_file, pixel_data, *callParams.trim_start))
    {
        return -1;
    }


    *callParams.trim_end = info.duration;

    //Maybe for displaying the total duration of the trimmed video? IDK where i'm using this
    double duration_seconds = info.duration * (double)info.time_base.num / (double)info.time_base.den;
    std::string human_time_duration = GetHumanTimeString(duration_seconds);



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


    

    GUIState guiState = {};
  

    while (!glfwWindowShouldClose(window))
    {
        CreateGuiInterface(guiState, callParams, info);
        
       

        switch (guiState.trimSliderState)
        {
        case ImGui::RangeSliderChangeType::FIRST_VALUE_CHANGED:

            if (!ReadFrame(callParams.input_file, pixel_data, *callParams.trim_start))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading frame at start time!");
                exit(1);
            }
            break;
        case ImGui::RangeSliderChangeType::SECOND_VALUE_CHANGED:

            if (!ReadFrame(callParams.input_file, pixel_data, *callParams.trim_end))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading frame at end time!");
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

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::UpdatePlatformWindows();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}



