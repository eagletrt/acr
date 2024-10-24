#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include "imgui.h"

class GUI {
public:
    GUI();
    ~GUI();

    // Set up ImGui and create a GLFW window
    bool setup();

    // Start a new ImGui frame
    void startFrame();

    // Render the ImGui frame and swap buffers
    void endFrame();

    // Clean up resources (new method)
    void cleanup();

    // Check if the window should close
    bool shouldClose() const;

    // Get the GLFW window
    GLFWwindow* getWindow() const;

private:
    GLFWwindow* window_;
};

#endif // GUI_HPP
