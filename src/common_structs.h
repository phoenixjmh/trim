#pragma once

#define MAX_PATH_BYTES 4096
#define OUTPUT_BUFFER_BYTES 9000

#ifndef NDEBUG
#define LOGD(...) \
    do { \
        std::cout << __VA_ARGS__ << "\n"; \
    } while (0)
#else
#define LOGD(...) 
#endif




struct SystemCallParameters
{
    uint32_t* trim_start;
    uint32_t* trim_end;
    uint32_t startTimeSeconds;
    uint32_t endTimeSeconds;
    char input_file [MAX_PATH_BYTES];
    char output_file[MAX_PATH_BYTES];
    char output_buffer[9000];
    bool custom_output_file = false;
};






namespace GUI
{
    enum class SliderState
    {
        NONE,
        FIRST_VALUE_CHANGED,
        SECOND_VALUE_CHANGED
    };
    struct GUIState
    {
        float dock_height;
        float dock_width;
        bool export_called = false;
        bool output_string_built = false;
        bool preview_output_command = false;
        bool display_editor_popup = false;
        SliderState sliderState;
        bool precision_seek = false;
        bool play_pushed = false;
        double playback_timestamp_seconds;
        double video_audio_time_difference=0;
    };

}