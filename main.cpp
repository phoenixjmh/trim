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
#include <unistd.h>
};

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
            // Update the viewport to match the new framebuffer size
            glViewport(0, 0, width, height);
        });
    


    return window;
}


std::string GetHumanTimeString(uint32_t time_seconds)
{

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d:%.2f", (int)floor((double)time_seconds / 60.0), std::floor(((((double)time_seconds / 60.0) - (floor(((double)time_seconds / 60.0)))) * 60) * 100.0) / 100.0);
    return{ buffer };
}

static int render_window_height;
static int render_window_width;
static  int minX, minY, maxX, maxY;

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
      "void main() {\n"
      "    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
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

void CreateVideoSurface(int windowW, int windowH, video_info& info) 
{

    float aspectRatio = (float)info.width_resolution / (float)info.height_resolution;
    float minY = 0, maxY = render_window_height;
    float frame_width = maxY * aspectRatio;
    float minX = (render_window_width - frame_width) / 2;
    float maxX = minX + frame_width;

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
    
    proj = glm::ortho(0.0f, (float)windowW, (float)windowH, 0.0f, -1.0f, 1.0f);
}


void DrawVideoFrame(GLuint tex,unsigned int shader_program) 
{
    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}



int main(int argc, char* argv[])
{
    char cwd[PATH_MAX];
       if (getcwd(cwd, sizeof(cwd)) != NULL) {
           printf("Current working dir: %s\n", cwd);
       }

    if (argc < 2)
    {
        PrintUsageError();
        return 1;
    }
    const char* input_file = argv[1];
    char output_file[256];
    if (argc > 2)
        strcpy(output_file, argv[2]);
    else
    {

        int response = processFileName(input_file, output_file);
        if (response != 0)
            return 1;
        std::cout << "Trimmed video will be written to " << output_file<<"\n";
    }

   


    GLFWwindow* window = InitGL();


    InitializeGUI(window);
    SetImGuiStyles();


    uint8_t* pixel_data;

    video_info info{};

    
    
    if (!ReadVideoMetaData(input_file, info))
    {
        return -1;
    }

    uint32_t* start_seek_time = new uint32_t(1);
    uint32_t* end_seek_time = new uint32_t(1);
    pixel_data = static_cast<uint8_t*>(malloc(info.width_resolution * info.height_resolution * 4));
    
    
    
    assert(pixel_data &&"Couldn't allocate framebuffer\n");

    if (!ReadFrame(input_file, pixel_data, *start_seek_time))
    {
        return -1;
    }

    double duration_seconds = info.duration * (double)info.time_base.num / (double)info.time_base.den;
    *end_seek_time = info.duration;
    std::string human_time_duration = GetHumanTimeString(duration_seconds);



    GLuint hTex;
    glGenTextures(1, &hTex);
    glBindTexture(GL_TEXTURE_2D, hTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


    unsigned int shader=CreateShaderProgram();


    while (!glfwWindowShouldClose(window))
    {
        static float dock_width, dock_height;


        auto startTimeSeconds = *start_seek_time * (double)info.time_base.num / (double)info.time_base.den;
        auto endTimeSeconds = *end_seek_time * (double)info.time_base.num / (double)info.time_base.den;
          
        //= GUI CODE ==//
        {



            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

            ImGui::Begin("Video Trimmer", nullptr, ImGuiWindowFlags_NoDecoration);
      
                     
            dock_height = ImGui::GetContentRegionAvail().y;
            dock_width = ImGui::GetContentRegionAvail().x;

            ImGui::Text("Trim Range Selection");
            ImGui::Separator();


            float fullWidth = ImGui::GetContentRegionAvail().x;


            ImGui::PushItemWidth(fullWidth);
            ImGui::RangeSliderChangeType change_type =
                ImGui::RangeSliderUInt("##TimeRange", start_seek_time, end_seek_time, 0, info.duration, "%.2f", 1);

            ImGui::PopItemWidth();
            ImGui::Text("Start Time: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", GetHumanTimeString(startTimeSeconds).c_str());

            ImGui::SameLine(200);
            ImGui::Text("End Time: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", GetHumanTimeString(endTimeSeconds).c_str());


            switch (change_type)
            {
            case ImGui::RangeSliderChangeType::FIRST_VALUE_CHANGED:

                if (!ReadFrame(input_file, pixel_data, *start_seek_time))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading frame at start time!");
                    exit(1);
                }
                break;
            case ImGui::RangeSliderChangeType::SECOND_VALUE_CHANGED:

                if (!ReadFrame(input_file, pixel_data, *end_seek_time))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading frame at end time!");
                    exit(1);
                }
                break;
            default:
                break;
            }

            ImGui::Separator();

            if (ImGui::Button("Export Video"))
            {
                
                std::string command = "ffmpeg -i \"" + std::string(input_file) +
                    "\" -ss " + GetHumanTimeString(startTimeSeconds) +
                    " -to " + GetHumanTimeString(endTimeSeconds) +
                    " -c copy \"" + std::string(output_file) + "\"";


                int result = std::system(command.c_str());

                if (result == 0)
                {
                    std::cout << "Video trimmed successfully: " << output_file << std::endl;
                }
                else
                {
                    std::cerr << "Error: ffmpeg command failed with code " << result << std::endl;
                }
                exit(0);

                ImGui::OpenPopup("Export");
            }

            if (ImGui::BeginPopupModal("Export", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Exporting your video...");
                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(120, 0)))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }


            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Adjust the sliders and click Export to save the trimmed video.");


            ImGui::End();


            ImGui::Render();

        }








        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width_resolution, info.height_resolution, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, pixel_data);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        int windowW, windowH;
        glfwGetWindowSize(window, &windowW, &windowH);


        render_window_height = windowH - dock_height;
        render_window_width = windowW;


        //Updating GL calls
        CreateVideoSurface(windowW,windowH,info);



        DrawVideoFrame(hTex,shader);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::UpdatePlatformWindows();
        glfwSwapBuffers(window);
        glfwPollEvents();
}
}



