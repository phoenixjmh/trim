#pragma once
#include "video_operations.h"
#include "common_structs.h"
#include "helpers.h"
#include "gl_includes.h"
#include "image_loader.h"


namespace GUI
{
   
    void Initialize(GLFWwindow* window);
    void SetStyles(float window_resolution);
    void CreateInterface(GUIState& guiState, SystemCallParameters& callParams, video_info& info);
    void TimelineSlider(SystemCallParameters& callParams, GUIState& guiState,  video_info& info,uint32_t slider_min,uint32_t slider_max);
    void EditCommandPopup(GUIState& state, SystemCallParameters& params);
    void Present();
    static Image PlayButton;
    static Image PauseButton;
    static Image ZoomInButton;
    static Image ZoomOutButton;

}
