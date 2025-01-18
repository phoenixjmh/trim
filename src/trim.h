#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <filesystem>


#include "video_operations.h"
#include "common_structs.h"
#include "gui.h"
#include "gl_helpers.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"




void CreateVideoSurface(const video_info& info, int w,int h,int xscale,int yscale);
void HandleVideoFrameResize(const video_info& info, int w,int h,int xscale, int yscale);
void ProcessArgs(int argc, char* argv[], SystemCallParameters& callParams);
void glfwErrorCallback(int error, const char* description);
GLFWwindow* InitGL();
bool ProcessFilename(const char* inFile, char* outFile);
void PrintUsageError();
void HandleVideoFrameResize(const video_info& info);
void DrawVideoFrame(GLuint tex, unsigned int shader_program);
void SendSystemCommand(const char* command);
void Export(SystemCallParameters& callParams, GUI::GUIState& guiState);
