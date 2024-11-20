
#include "gui.hpp"
#include <string.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <queue> 
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>  
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "stb_image.h"
#include "nfd.h"

#include "gps.hpp"
#include "file_browser.hpp"
#include "notifications.hpp"
#include "map.hpp"
#include "font_manager.hpp"
#include "icon_manager.hpp"
#include "utils.hpp"
#include "cones_loader.hpp"

extern "C" {
    #include "acr.h"
    #include "defines.h"
    #include "main.h"
    #include "utils.h"
}

#include "notifications.hpp"
#include "icon_manager.hpp"


static void setEnhancedTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]             = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.33f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]            = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.33f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.33f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.33f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_Separator]            = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.40f, 0.43f, 0.47f, 1.00f);
    colors[ImGuiCol_SeparatorActive]      = ImVec4(0.50f, 0.53f, 0.57f, 1.00f);
    colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_Border]               = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_Tab]                  = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]           = ImVec4(0.28f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive]            = ImVec4(0.33f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_TabUnfocused]         = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_PlotLines]            = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]        = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]    = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_TableBorderLight]     = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_TableRowBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]        = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    
    style.WindowRounding     = 8.0f;
    style.FrameRounding      = 6.0f;
    style.ScrollbarRounding  = 12.0f;
    style.GrabRounding       = 4.0f;
    style.FramePadding       = ImVec2(8.0f, 6.0f);
    style.ItemSpacing        = ImVec2(12.0f, 8.0f);
    style.IndentSpacing      = 25.0f;
    style.ScrollbarSize      = 18.0f;
    style.WindowPadding      = ImVec2(12.0f, 12.0f);
    style.Alpha              = 1.0f; 
}

GUI::GUI()
    : window_(nullptr) {}

GUI::~GUI() {
    cleanup();
}

bool GUI::setup() {
    if (!glfwInit()) {
        return false;
    }

    
    int windowWidth = 1280;
    int windowHeight = 720;

    
    window_ = glfwCreateWindow(windowWidth, windowHeight, "ACR", nullptr, nullptr);
    if (window_ == nullptr)
        return false;
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); 

    
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 

    
    setEnhancedTheme();

    
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL2_Init();

    return true;
}


void GUI::startFrame() {
    glfwPollEvents();

    
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUI::endFrame() {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.10f, 0.10f, 1.00f); 
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window_);
    glfwSwapBuffers(window_);
}

void GUI::cleanup() {
    
    if (window_) {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr; 
    }
}

bool GUI::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

GLFWwindow* GUI::getWindow() const {
    return window_;
}

void ApplyTheme(AppTheme theme) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    switch (theme) {
        case AppTheme::Dark: {
            
            ImGui::StyleColorsDark();

            colors[ImGuiCol_Button]               = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_ButtonHovered]        = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_ButtonActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            colors[ImGuiCol_FrameBg]              = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_FrameBgActive]        = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            colors[ImGuiCol_Header]               = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_HeaderHovered]        = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_HeaderActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            colors[ImGuiCol_Separator]            = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
            colors[ImGuiCol_SeparatorActive]      = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            colors[ImGuiCol_Tab]                  = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TabHovered]           = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            colors[ImGuiCol_TabActive]            = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            colors[ImGuiCol_TabUnfocused]         = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);

            colors[ImGuiCol_Text]                 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            break;
        }
        case AppTheme::Blue: {
            
            ImGui::StyleColorsDark();

            ImVec4 darkBlue = ImVec4(0.0f, 0.0f, 0.4f, 1.0f);        
            ImVec4 darkBlueHovered = ImVec4(0.0f, 0.0f, 0.5f, 1.0f); 
            ImVec4 darkBlueActive = ImVec4(0.0f, 0.0f, 0.6f, 1.0f);  

            colors[ImGuiCol_Button]               = darkBlue;
            colors[ImGuiCol_ButtonHovered]        = darkBlueHovered;
            colors[ImGuiCol_ButtonActive]         = darkBlueActive;
            colors[ImGuiCol_FrameBg]              = darkBlue;
            colors[ImGuiCol_FrameBgHovered]       = darkBlueHovered;
            colors[ImGuiCol_FrameBgActive]        = darkBlueActive;
            colors[ImGuiCol_Header]               = darkBlue;
            colors[ImGuiCol_HeaderHovered]        = darkBlueHovered;
            colors[ImGuiCol_HeaderActive]         = darkBlueActive;
            colors[ImGuiCol_Separator]            = darkBlue;
            colors[ImGuiCol_SeparatorHovered]     = darkBlueHovered;
            colors[ImGuiCol_SeparatorActive]      = darkBlueActive;
            colors[ImGuiCol_ScrollbarGrab]        = darkBlue;
            colors[ImGuiCol_ScrollbarGrabHovered] = darkBlueHovered;
            colors[ImGuiCol_ScrollbarGrabActive]  = darkBlueActive;
            colors[ImGuiCol_Tab]                  = darkBlue;
            colors[ImGuiCol_TabHovered]           = darkBlueHovered;
            colors[ImGuiCol_TabActive]            = darkBlueActive;
            colors[ImGuiCol_TabUnfocused]         = darkBlue;
            colors[ImGuiCol_TabUnfocusedActive]   = darkBlueActive;

            colors[ImGuiCol_Text]                 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

            break;
        }
        case AppTheme::Light: {
            
            ImGui::StyleColorsLight();

            ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 grayHovered = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
            ImVec4 grayActive = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

            colors[ImGuiCol_Button]               = white;
            colors[ImGuiCol_ButtonHovered]        = grayHovered;
            colors[ImGuiCol_ButtonActive]         = grayActive;
            colors[ImGuiCol_FrameBg]              = white;
            colors[ImGuiCol_FrameBgHovered]       = grayHovered;
            colors[ImGuiCol_FrameBgActive]        = grayActive;
            colors[ImGuiCol_Header]               = white;
            colors[ImGuiCol_HeaderHovered]        = grayHovered;
            colors[ImGuiCol_HeaderActive]         = grayActive;
            colors[ImGuiCol_Separator]            = grayActive;
            colors[ImGuiCol_SeparatorHovered]     = grayHovered;
            colors[ImGuiCol_SeparatorActive]      = grayActive;
            colors[ImGuiCol_ScrollbarGrab]        = grayActive;
            colors[ImGuiCol_ScrollbarGrabHovered] = grayHovered;
            colors[ImGuiCol_ScrollbarGrabActive]  = grayActive;
            colors[ImGuiCol_Tab]                  = white;
            colors[ImGuiCol_TabHovered]           = grayHovered;
            colors[ImGuiCol_TabActive]            = grayActive;
            colors[ImGuiCol_TabUnfocused]         = white;
            colors[ImGuiCol_TabUnfocusedActive]   = grayActive;

            colors[ImGuiCol_Text]                 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            colors[ImGuiCol_TextDisabled]         = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

            break;
        }
    }

    
    style.WindowRounding     = 8.0f;
    style.FrameRounding      = 6.0f;
    style.ScrollbarRounding  = 12.0f;
    style.GrabRounding       = 4.0f;
    style.FramePadding       = ImVec2(8.0f, 6.0f);
    style.ItemSpacing        = ImVec2(12.0f, 8.0f);
    style.IndentSpacing      = 25.0f;
    style.ScrollbarSize      = 18.0f;
    style.WindowPadding      = ImVec2(12.0f, 12.0f);
    style.Alpha              = 1.0f; 
}

void SetImPlotStyle(AppTheme theme) {
    ImPlotStyle& plotStyle = ImPlot::GetStyle();

    
    plotStyle = ImPlotStyle();

    switch (theme) {
        case AppTheme::Dark:
            plotStyle.Colors[ImPlotCol_Line]        = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); 
            plotStyle.Colors[ImPlotCol_Fill]        = ImVec4(0.0f, 1.0f, 0.0f, 0.3f);
            plotStyle.Colors[ImPlotCol_MarkerFill]  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            plotStyle.Colors[ImPlotCol_AxisGrid]    = ImVec4(0.5f, 0.5f, 0.5f, 0.3f);
            plotStyle.Colors[ImPlotCol_LegendBg]    = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
            plotStyle.Colors[ImPlotCol_TitleText]   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case AppTheme::Blue:
            plotStyle.Colors[ImPlotCol_Line]        = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); 
            plotStyle.Colors[ImPlotCol_Fill]        = ImVec4(0.0f, 0.0f, 1.0f, 0.3f);
            plotStyle.Colors[ImPlotCol_MarkerFill]  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            plotStyle.Colors[ImPlotCol_AxisGrid]    = ImVec4(0.3f, 0.3f, 0.5f, 0.3f);
            plotStyle.Colors[ImPlotCol_LegendBg]    = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
            plotStyle.Colors[ImPlotCol_TitleText]   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case AppTheme::Light:
            plotStyle.Colors[ImPlotCol_Line]        = ImVec4(0.0f, 0.8f, 0.0f, 1.0f); 
            plotStyle.Colors[ImPlotCol_Fill]        = ImVec4(0.0f, 0.8f, 0.0f, 0.3f);
            plotStyle.Colors[ImPlotCol_MarkerFill]  = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            plotStyle.Colors[ImPlotCol_AxisGrid]    = ImVec4(0.8f, 0.8f, 0.8f, 0.3f);
            plotStyle.Colors[ImPlotCol_LegendBg]    = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            plotStyle.Colors[ImPlotCol_TitleText]   = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            break;
    }

    
    plotStyle.LineWeight = 2.0f;
    plotStyle.MarkerSize = 6.0f;
}

bool LoadConfig(AppTheme& theme) {
    std::ifstream configFile("config.ini");
    if (!configFile.is_open()) {
        
        theme = AppTheme::Dark;
        return false;
    }
    std::string line;
    bool themeSet = false;
    while (std::getline(configFile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "theme") {
                if (value == "Dark") {
                    theme = AppTheme::Dark;
                    themeSet = true;
                } else if (value == "Blue") { 
                    theme = AppTheme::Blue;
                    themeSet = true;
                } else if (value == "Light") {
                    theme = AppTheme::Light;
                    themeSet = true;
                }
            }
        }
    }
    configFile.close();
    if (!themeSet) {
        
        theme = AppTheme::Dark;
    }
    return themeSet;
}

bool SaveConfig(const AppTheme& theme) {
    std::ofstream configFile("config.ini", std::ios::out | std::ios::trunc);
    if (!configFile.is_open()) {
        printf("Failed to open config file for writing.\n");
        return false;
    }
    
    std::string themeStr;
    switch (theme) {
        case AppTheme::Dark:
            themeStr = "Dark";
            break;
        case AppTheme::Blue:
            themeStr = "Blue";
            break;
        case AppTheme::Light:
            themeStr = "Light";
            break;
    }
    configFile << "theme=" << themeStr << "\n";
    configFile.close();
    return true;
}

