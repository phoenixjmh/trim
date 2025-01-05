#pragma once
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstdio>
#include <cassert>
#include <filesystem>


#include "video_operations.h"
#include "common_structs.h"
#include "gui.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"




void CreateVideoSurface(const video_info& info);
void HandleVideoFrameResize(const video_info& info);
void ProcessArgs(int argc, char* argv[], SystemCallParameters& callParams);
void glfwErrorCallback(int error, const char* description);
GLFWwindow* InitGL();
bool ProcessFilename(const char* inFile, char* outFile);
void PrintUsageError();
void HandleVideoFrameResize(const video_info& info);
unsigned int CreateShaderProgram();
void DrawVideoFrame(GLuint tex, unsigned int shader_program);
void SendSystemCommand(const char* command);
void Export(SystemCallParameters& callParams, GUI::GUIState& guiState);