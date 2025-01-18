#pragma once
#include "video_operations.h"
#include "common_structs.h"
#include "helpers.h"
#include "gl_includes.h"


namespace GUI
{
   
    void Initialize(GLFWwindow* window);
    void SetStyles();
    void CreateInterface(GUIState& guiState, SystemCallParameters& callParams, const video_info& info);
    void TimelineSlider(SystemCallParameters& callParams, GUIState& guiState, const video_info& info);
    void EditCommandPopup(GUIState& state, SystemCallParameters& params);
    void Present();


}
