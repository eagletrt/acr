#include "gui.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "font_manager.hpp"
#include <cstdio>
#include "notifications.hpp"
#include "implot.h"
#include <GL/gl.h>

// Function to set a custom ImGui theme
static void setCustomTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Base colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_MenuBarBg]            = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Separator]            = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.40f, 0.43f, 0.47f, 1.00f);
    colors[ImGuiCol_SeparatorActive]      = ImVec4(0.50f, 0.53f, 0.57f, 1.00f);
    colors[ImGuiCol_Text]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Adjust style variables
    style.WindowRounding    = 5.0f;
    style.FrameRounding     = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding      = 5.0f;
    style.FramePadding      = ImVec2(5.0f, 5.0f);
    style.ItemSpacing       = ImVec2(10.0f, 10.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 15.0f;
    style.WindowPadding     = ImVec2(0.0f, 0.0f); // Removed padding for fullscreen
}

GUI::GUI()
    : window_(nullptr) {}

GUI::~GUI() {
    // Cleanup ImGui and GLFW
    if (window_) {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
}

// Function to set up ImGui and create a GLFW window
bool GUI::setup() {
    if (!glfwInit())
        return false;

    // Create fullscreen window by passing the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    window_ = glfwCreateWindow(mode->width, mode->height, "ACR", nullptr, nullptr);
    if (window_ == nullptr)
        return false;
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Apply custom theme
    setCustomTheme();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL2_Init();

    return true;
}

// Start a new ImGui frame
void GUI::startFrame() {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// Render the ImGui frame and swap buffers
void GUI::endFrame() {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.10f, 0.10f, 1.00f); // Dark background
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window_);
    glfwSwapBuffers(window_);
}

void GUI::cleanup() {
    // Cleanup ImGui and GLFW
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

// Check if the window should close
bool GUI::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

// Get the GLFW window
GLFWwindow* GUI::getWindow() const {
    return window_;
}
