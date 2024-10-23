#ifndef FONT_MANAGER_HPP
#define FONT_MANAGER_HPP

#include <string>
#include <vector>
#include "imgui/imgui.h"

// Structure for font information
struct FontInfo {
    std::string name;
    ImFont* font;
};

// External variable declarations
extern std::vector<FontInfo> availableFonts;
extern int selectedFontIndex; // Index of the selected font
extern float fontScale; 

// Function declarations
void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir);
void initializeFonts(ImGuiIO& io, const std::string& fontsDir);

#endif // FONT_MANAGER_HPP
