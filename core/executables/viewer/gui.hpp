
#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include "imgui.h"


enum class AppTheme {
    Dark,
    Blue,
    Light
};


void ApplyTheme(AppTheme theme);
bool LoadConfig(AppTheme& theme);
bool SaveConfig(const AppTheme& theme);


class GUI {
public:
    GUI();
    ~GUI();

    
    bool setup();

    
    void startFrame();

    
    void endFrame();

    
    void cleanup();

    
    bool shouldClose() const;

    
    GLFWwindow* getWindow() const;

private:
    GLFWwindow* window_;
};


void SetImPlotStyle(AppTheme theme);

#endif 
