#ifndef FONT_MANAGER_HPP
#define FONT_MANAGER_HPP

#include <string>
#include <vector>
#include "imgui.h"

// Structure for font information
struct FontInfo {
    std::string name;
    ImFont* font;
};

class FontManager {
public:
    FontManager();
    ~FontManager();

    // Load fonts from directory
    void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir);

    // Initialize fonts
    void initializeFonts(ImGuiIO& io, const std::string& fontsDir);

    // Get available fonts
    const std::vector<FontInfo>& getAvailableFonts() const;

    // Get and set selected font index
    int getSelectedFontIndex() const;
    void setSelectedFontIndex(int index);

    // Get and set font scale
    float getFontScale() const;
    void setFontScale(float scale);

    // Get the selected font
    ImFont* getSelectedFont() const;
    float fontScale_ = 1.0f;

private:
    std::vector<FontInfo> availableFonts_;
    int selectedFontIndex_;
    
};

#endif // FONT_MANAGER_HPP
