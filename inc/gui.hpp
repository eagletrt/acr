#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include "imgui.h"

// Enumerazione per i temi disponibili
enum class AppTheme {
    Dark,
    Blue,
    Light
};

// Dichiarazioni delle funzioni di tema e configurazione
void ApplyTheme(AppTheme theme);
bool LoadConfig(AppTheme& theme);
bool SaveConfig(const AppTheme& theme);

class GUI {
public:
    GUI();
    ~GUI();

    // Configura ImGui e crea una finestra GLFW
    bool setup();

    // Inizia un nuovo frame ImGui
    void startFrame();

    // Renderizza il frame ImGui e scambia i buffer
    void endFrame();

    // Pulisce le risorse
    void cleanup();

    // Verifica se la finestra deve chiudersi
    bool shouldClose() const;

    // Ottiene la finestra GLFW
    GLFWwindow* getWindow() const;

private:
    GLFWwindow* window_;
};

#endif
