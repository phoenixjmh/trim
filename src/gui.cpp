#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "range_slider.h"

#include <string>
#include "video_operations.h"
#include "gui.h"
#include <glm/glm.hpp>
#include "helpers.h"
#include <format>

#include "image_loader.h"

namespace GUI
{
    void Initialize(GLFWwindow *window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle &style = ImGui::GetStyle();
        if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();

         float xscale, yscale;
        int width,height;
        glfwGetWindowSize(window,&width,&height);

        glfwGetWindowContentScale(window, &xscale, &yscale);
        int window_resolution = (float)height * yscale;

        SetStyles(window_resolution);
        //Init resources
        PlayButton = LoadTexture("../res/play_button.png");
        PauseButton = LoadTexture("../res/pause_button.png");
        ZoomInButton = LoadTexture("../res/zoom_button.png");
        ZoomOutButton = LoadTexture("../res/zoom_out_button.png");





    }

    void DockToBottom(ImGuiID dock_space)
    {

        static bool first_run = true;
        if (first_run)
        {
            first_run = false;
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::DockBuilderRemoveNode(dock_space);
            ImGui::DockBuilderAddNode(
                dock_space, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dock_space, viewport->Size);

            // Split off the bottom section only
            ImGuiID dock_bottom;
            ImGui::DockBuilderSplitNode(dock_space, ImGuiDir_Down, 0.4f, &dock_bottom, &dock_space);

            // Dock the Video Trimmer window
            ImGui::DockBuilderDockWindow("Video Trimmer", dock_bottom);
            ImGui::DockBuilderFinish(dock_space);
        }
    }

    ImVec2 GetCenterX(float panel_width, float item_width = 0)
    {
        return ImVec2((panel_width * 0.5f) - (item_width * 0.5), ImGui::GetCursorPosY());
    }

    void CreateInterface(GUIState &guiState, SystemCallParameters &callParams, video_info &info)
    {
        callParams.startTimeSeconds = ToSeconds(*callParams.trim_start, info.VideoTimeBase);
        callParams.endTimeSeconds = ToSeconds(*callParams.trim_end, info.VideoTimeBase);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiID dock_space =
                ImGui::DockSpaceOverViewport(
                    ImGui::GetMainViewport(),
                    ImGuiDockNodeFlags_PassthruCentralNode
                    | ImGuiDockNodeFlags_AutoHideTabBar);

        DockToBottom(dock_space);

        ImGui::Begin("Video Trimmer", nullptr, ImGuiWindowFlags_NoDecoration);

        guiState.dock_width = ImGui::GetWindowWidth();
        guiState.dock_height = ImGui::GetContentRegionAvail().y;


        static uint32_t slider_min;
        static uint32_t slider_max;
        static bool zoomedToRange = false;



        //Timeline buttons
        {
            int panel_width = 25 * 5;

            const ImVec2 button_size = {15, 18};

            ImVec2 windowCenter = GetCenterX(guiState.dock_width, panel_width);
            ImGui::SetCursorPos(windowCenter);
            if (ImGui::ImageButton("#zo", (void *) (intptr_t) ZoomOutButton.ID, button_size))
            {
                zoomedToRange = false;
            }
            ImGui::SameLine();


            //Play Button

            auto current_image = guiState.isPlaying ? PauseButton : PlayButton;
            if (ImGui::ImageButton("#pb", (void *) (intptr_t) current_image.ID, button_size))
            {
                guiState.play_pushed = !guiState.play_pushed;
            }

            ImGui::SameLine();


            if (ImGui::ImageButton("#zi", (void *) (intptr_t) ZoomInButton.ID, button_size))
            {
                zoomedToRange = true;
                slider_min = *callParams.trim_start;
                slider_max = *callParams.trim_end;
            }
            if (!zoomedToRange)
            {
                slider_min = 0;
                slider_max = info.duration;
            }
        }
        ImGui::Separator();

        //Playback timestamp

        //Start end time
        {

            auto HumanTimeStart = GetHumanTimeString(callParams.startTimeSeconds).c_str();
            auto HumanTimeEnd = GetHumanTimeString(callParams.endTimeSeconds).c_str();
            auto HumanTimeCurrent = GetHumanTimeString(guiState.playback_timestamp_seconds).c_str();
            auto TimeWidth = ImGui::CalcTextSize(HumanTimeStart).x;

            ImGui::TextColored(ImVec4(0.2f, 0.2f, 1.f, 1.0f), "%s", HumanTimeStart); {
                ImGui::SameLine();
                const char *label = HumanTimeCurrent;
                ImGui::SetCursorPosX((guiState.dock_width * 0.5) - (TimeWidth * 0.5));
                ImGui::Text("%s", label);
            }

            ImGui::SameLine();
            ImGui::SetCursorPosX(guiState.dock_width - (TimeWidth + 10));
            ImGui::TextColored(ImVec4(0.2f, 0.2f, 1.f, 1.0f), "%s", HumanTimeEnd);
        }


        // Timeline slider for video trimming
        TimelineSlider(callParams, guiState, info, slider_min, slider_max);

        // End behavior of range slider
        ImGui::PopItemWidth();


        //Export button
        {
            ImVec2 ExportButtonSize={guiState.dock_width/5,guiState.dock_height/5};
            ImGui::SetCursorPosX(guiState.dock_width * 0.5 - ExportButtonSize.x * 0.5);
            if (ImGui::Button("Export Video",ExportButtonSize))
            {
                guiState.export_called = true;
                if (guiState.preview_output_command)
                    guiState.display_editor_popup = true;
            }
        }

        EditCommandPopup(guiState, callParams);

        //Stats panel
        {
            ImGui::Separator();

            ImVec2 windowRight = ImVec2(guiState.dock_width, guiState.dock_height);
            ImVec2 windowLeft = ImVec2(0, guiState.dock_height);
            // Preview Checkbox
            {
                const char *cb_label = "Preview output before export";
                ImVec2 cb_size = ImGui::CalcTextSize(cb_label);

                ImGui::SetCursorPos(windowLeft);
                ImGui::Checkbox("Preview output before export", &guiState.preview_output_command);
                ImGui::SameLine();
            }

            auto io = ImGui::GetIO();

            // Show FPS and time difference stats at the bottom

            auto fps_fmt_lbl= "%.3f ms/frame (%.1f FPS)";
            auto ts_fmt_lbl = "Video Audio time difference: %.3f";
            auto stats_width =
                ImGui::CalcTextSize(fps_fmt_lbl).x
            +   ImGui::CalcTextSize(ts_fmt_lbl).x
            +   ImGui::CalcTextSize("0.0000").x;

            ImVec2 infoStartPos = {windowRight.x - stats_width, windowRight.y};
            ImGui::SetCursorPos(infoStartPos);
            ImGui::Text(fps_fmt_lbl, 1000.0f / io.Framerate, io.Framerate);
            ImGui::SameLine();
            ImGui::Text(ts_fmt_lbl, guiState.video_audio_time_difference);
        }
        ImGui::End();
        ImGui::Render();

    }

    void EditCommandPopup(GUIState &state, SystemCallParameters &callParams)
    {
        if (!state.display_editor_popup)
            return;
        ImGui::OpenPopup("Edit Command");
        if (ImGui::BeginPopupModal("Edit Command", nullptr, ImGuiWindowFlags_None))
        {
            ImGui::InputTextMultiline("Final command", callParams.output_buffer, 1024,
                                      ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16));
            ImGui::Separator();

            if (ImGui::Button("Finalize & Export", ImVec2(200, 0)))
            {
                ImGui::CloseCurrentPopup();
                state.display_editor_popup = false;
                state.preview_output_command = false;
            }
            ImGui::SameLine(200);
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                state.export_called = false;
                state.output_string_built = false;
                state.display_editor_popup = false;
            }

            ImGui::EndPopup();
        }
    }

    void Present()
    {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::UpdatePlatformWindows();
    }

    void TimelineSlider(
        SystemCallParameters &callParams, GUIState &guiState, video_info &info,
        uint32_t slider_min, uint32_t slider_max)
    {
        //TODO: Zoom in to range?
        float fullWidth = ImGui::GetContentRegionAvail().x;
        ImGui::PushItemWidth(fullWidth);
        ImGui::PushItemWidth(fullWidth);

        //ImGui::ImageButton()

        ImGui::RangeSliderChangeType change_type = ImGui::RangeSliderChangeType::NONE;
        // TODO:Unfuck this
        static double holdTimeSinceChange = 0;
        static double timeOfValueChange = 0;
        static bool HighPrecision = false;
        static bool slider_value_change = false;
        bool is_animating_slider = false;

        // Smooth animation indicating precision mode
        static float currentPadding = 5.0f;
        static float currentGrabSize = 20.0f;
        currentGrabSize = glm::mix(currentGrabSize, HighPrecision ? 5.0f : 20.0f, 0.15f);
        currentPadding = glm::mix(currentPadding, HighPrecision ? 10.0f : 5.0f, 0.15f);

        // check animation state
        if (currentGrabSize > 6.0f && currentGrabSize < 19.0f)
            is_animating_slider = true;



        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, currentPadding));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, currentGrabSize);

        uint32_t display_time = callParams.trim_end - callParams.trim_start;

        if (ImGui::RangeSliderUInt(
            "Time Range",
            callParams.trim_start, callParams.trim_end,
            slider_min, slider_max, "%.2f", 1,
            change_type, info.VideoTimeBase.num, info.VideoTimeBase.den))
        {
            timeOfValueChange = ImGui::GetTime();
            auto otherval = glfwGetTime();
        }

        if (ImGui::IsItemActivated())
        {
            holdTimeSinceChange = 0;
            timeOfValueChange = ImGui::GetTime();
        }
        if (ImGui::IsItemActive())
        {
            // How long is slider held, but stationary?
            holdTimeSinceChange = ImGui::GetTime() - timeOfValueChange;
            if (holdTimeSinceChange > 1.0)
            {
                HighPrecision = true;
            }
        }
        if (ImGui::IsItemDeactivated())
        {
            holdTimeSinceChange = 0;
            HighPrecision = false;
        }

        ImGui::PopStyleVar(2); // Restore padding and grab size

        guiState.sliderState = (SliderState) change_type;

        if (!is_animating_slider && HighPrecision)
        {
            guiState.precision_seek = true;
            std::cout << "Precision seek\n";
        } else
        {
            guiState.precision_seek = false;
        }
    }

    void SetStyles(float window_resolution)
    {
        ImGuiIO &io = ImGui::GetIO();
        ImVec4 *colors = ImGui::GetStyle().Colors;



        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8.00f, 8.00f);
        style.FramePadding = ImVec2(5.00f, 2.00f);
        style.CellPadding = ImVec2(6.00f, 6.00f);
        style.ItemSpacing = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
        style.IndentSpacing = 25;
        style.ScrollbarSize = 15;
        style.GrabMinSize = 10;
        style.WindowBorderSize = 1;
        style.ChildBorderSize = 1;
        style.PopupBorderSize = 1;
        style.FrameBorderSize = 1;
        style.TabBorderSize = 1;
        style.WindowRounding = 7;
        style.ChildRounding = 4;
        style.FrameRounding = 3;
        style.PopupRounding = 4;
        style.ScrollbarRounding = 9;
        style.GrabRounding = 3;
        style.LogSliderDeadzone = 4;
        style.TabRounding = 4;





        ImGui::GetStyle().ScaleAllSizes(2.f);
        //TODO: scale to screen resolution

        float fontsize=window_resolution*0.014;

        io.Fonts->AddFontFromFileTTF("../res/OpenSans.ttf",fontsize);

        //ImGui::PushFont(OpenSans);

        //ImGui::PopFont();
    }
}
