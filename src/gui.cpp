
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "range_slider.h"

#include <string>
#include "video_operations.h"
#include "gui.h"
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include "helpers.h"
namespace GUI
{





    void Initialize(GLFWwindow* window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        //io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }



    void DockToBottom(ImGuiID dock_space)
    {
        static bool first_run = true;
        if (first_run)
        {
            first_run = false;
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::DockBuilderRemoveNode(dock_space);
            ImGui::DockBuilderAddNode(dock_space, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dock_space, viewport->Size);

            // Split off the bottom section only
            ImGuiID dock_bottom;
            ImGui::DockBuilderSplitNode(dock_space, ImGuiDir_Down, 0.4f, &dock_bottom, &dock_space);

            // Dock the Video Trimmer window
            ImGui::DockBuilderDockWindow("Video Trimmer", dock_bottom);
            ImGui::DockBuilderFinish(dock_space);
        }
    }

    void CreateInterface(GUIState& guiState, SystemCallParameters& callParams, const video_info& info)
    {
        callParams.startTimeSeconds = *callParams.trim_start * (double)info.VideoTimeBase.num / (double)info.VideoTimeBase.den;
        callParams.endTimeSeconds = *callParams.trim_end * (double)info.VideoTimeBase.num / (double)info.VideoTimeBase.den;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiID dock_space = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

        DockToBottom(dock_space);

        ImGui::Begin("Video Trimmer", nullptr, ImGuiWindowFlags_NoDecoration);
        if (ImGui::Button("Play", { 200,200 }))
        {
            guiState.play_pushed = !guiState.play_pushed;
        }
        ImGui::Text(GetHumanTimeString(guiState.playback_timestamp_seconds).c_str());



        guiState.dock_height = ImGui::GetContentRegionAvail().y;
        guiState.dock_width = ImGui::GetContentRegionAvail().x;

        ImGui::Text("Trim Range Selection");
        ImGui::Separator();


        float fullWidth = ImGui::GetContentRegionAvail().x;


        ImGui::PushItemWidth(fullWidth);


        TimelineSlider(callParams, guiState, info);


        //end behavior of range slider
        ImGui::PopItemWidth();
        ImGui::Text("Start Time: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", GetHumanTimeString(callParams.startTimeSeconds).c_str());

        ImGui::SameLine(200);
        ImGui::Text("End Time: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", GetHumanTimeString(callParams.endTimeSeconds).c_str());


        ImGui::Separator();
        ImGui::Checkbox("Preview output before export", &guiState.preview_output_command);
        if (ImGui::Button("Export Video"))
        {
            guiState.export_called = true;
            if (guiState.preview_output_command)
                guiState.display_editor_popup = true;
        }


        EditCommandPopup(guiState, callParams);




        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Adjust the sliders and click Export to save the trimmed video.");

        auto io = ImGui::GetIO();
        ImGui::Separator();
        ImGui::Text(" %.3f ms/frame (%.1f FPS)",
            1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
        ImGui::Render();
    }


    void EditCommandPopup(GUIState& state, SystemCallParameters& callParams)
    {
        if (!state.display_editor_popup)
            return;
        ImGui::OpenPopup("Edit Command");
        if (ImGui::BeginPopupModal("Edit Command", nullptr, ImGuiWindowFlags_None))
        {

            ImGui::InputTextMultiline("Final command", callParams.output_buffer, 1024, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 16));
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

    void TimelineSlider(SystemCallParameters& callParams, GUIState& guiState, const video_info& info)
    {
        ImGui::RangeSliderChangeType change_type = ImGui::RangeSliderChangeType::NONE;
        //TODO:Unfuck this
        static double holdTimeSinceChange = 0;
        static double timeOfValueChange = 0;
        static bool HighPrecision = false;
        static bool slider_value_change = false;

        bool is_animating_slider = false;

        //Smooth animation indicating precision mode
        static float currentPadding = 5.0f;
        static float currentGrabSize = 20.0f;
        currentGrabSize = glm::mix(currentGrabSize, HighPrecision ? 5.0f : 20.0f, 0.15f);
        currentPadding = glm::mix(currentPadding, HighPrecision ? 10.0f : 5.0f, 0.15f);

        //check animation state
        if (currentGrabSize > 6.0f && currentGrabSize < 19.0f)
            is_animating_slider = true;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, currentPadding));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, currentGrabSize);

        uint32_t display_time = callParams.trim_end - callParams.trim_start;

        if (ImGui::RangeSliderUInt(
            "Time Range",
            callParams.trim_start, callParams.trim_end,
            0, info.duration, "%.2f", 1,
            change_type, info.VideoTimeBase.num, info.VideoTimeBase.den))
        {
            timeOfValueChange = ImGui::GetTime();
        }

        if (ImGui::IsItemActivated())
        {
            holdTimeSinceChange = 0;
            timeOfValueChange = ImGui::GetTime();
        }
        if (ImGui::IsItemActive())
        {
            //How long is slider held, but stationary?
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



        ImGui::PopStyleVar(2);      // Restore padding and grab size

        guiState.sliderState = (SliderState)change_type;

        if (!is_animating_slider && HighPrecision)
        {
            guiState.precision_seek = true;

        }
        else
        {
            guiState.precision_seek = false;
        }

    }


    void SetStyles()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImVec4* colors = ImGui::GetStyle().Colors;
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

        ImGuiStyle& style = ImGui::GetStyle();
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
        //io.Fonts->AddFontFromFileTTF("external/imgui/misc/fonts/Roboto-Medium.ttf", 13);
        ImGui::GetStyle().ScaleAllSizes(2.f);
    }
}
