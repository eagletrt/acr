#ifndef FONT_MANAGER_HPP
#define FONT_MANAGER_HPP

#include <string>
#include <vector>
#include "imgui/imgui.h"

// Struttura per le informazioni sulla font
struct FontInfo {
    std::string name;
    ImFont* font;
};

// Dichiarazione delle variabili esterne
extern std::vector<FontInfo> availableFonts;
extern int selectedFontIndex; // Indice della font selezionata
extern float fontScale; 

// Dichiarazione delle funzioni
void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir);
void initializeFonts(ImGuiIO& io, const std::string& fontsDir);

#endif // FONT_MANAGER_HPP
