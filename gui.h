#pragma once
#include "video_operations.h"
#include "common_structs.h"
//trim_start & trim_end : libav raw time stamp
//startTimeSeconds & endTimeSeconds : raw time translated to seconds
//output_buffer : The working buffer for the output command to ffmpeg




namespace GUI
{
   
  
   
   
    void Initialize(GLFWwindow* window);
    void SetStyles();
    void CreateInterface(GUIState& guiState, SystemCallParameters& callParams, const video_info& info);
    void TimelineSlider(SystemCallParameters& callParams, GUIState& guiState, const video_info& info);
    void EditCommandPopup(GUIState& state, SystemCallParameters& params);
    void Present();


    std::string GetHumanTimeString(uint32_t time_seconds);
}

