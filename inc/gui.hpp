#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include "imgui.h"

// Enumeration for available themes
enum class AppTheme {
    Dark,
    Blue,
    Light
};

// Declarations for theme and configuration functions
void ApplyTheme(AppTheme theme);
bool LoadConfig(AppTheme& theme);
bool SaveConfig(const AppTheme& theme);

// Class for GUI management
class GUI {
public:
    GUI();
    ~GUI();

    // Configure ImGui and create a GLFW window
    bool setup();

    // Start a new ImGui frame
    void startFrame();

    // Render the ImGui frame and swap buffers
    void endFrame();

    // Clean up resources
    void cleanup();

    // Check if the window should close
    bool shouldClose() const;

    // Get the GLFW window
    GLFWwindow* getWindow() const;

private:
    GLFWwindow* window_;
};

// Function to set ImPlot style based on theme
void SetImPlotStyle(AppTheme theme);

#endif // GUI_HPP
