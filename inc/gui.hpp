
#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include "imgui/imgui.h"

// Function to set up ImGui and create a GLFW window
GLFWwindow *setupImGui();

// Function to start a new ImGui frame
void startFrame();
void endFrame(GLFWwindow* window);
#endif