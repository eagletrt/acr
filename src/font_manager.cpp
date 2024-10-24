#include "font_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include "notifications.hpp"

FontManager::FontManager()
    : selectedFontIndex_(0), fontScale_(1.0f) {}

FontManager::~FontManager() {}

// Function to load fonts from the directory
void FontManager::loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir) {
    // Supported font file extensions
    std::vector<std::string> extensions = { ".ttf", ".otf" };

    for (const auto& entry : std::filesystem::directory_iterator(fontsDir)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();
            // Check if the file has a supported extension
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                // Extract the font name from the file name
                std::string name = entry.path().stem().string();
                // Load the font with the default size
                ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
                if (font) {
                    availableFonts_.push_back(FontInfo{ name, font });
                    printf("Font loaded: %s\n", name.c_str());
                }
                else {
                    printf("Unable to load font: %s\n", path.c_str());
                }
            }
        }
    }

    if (availableFonts_.empty()) {
        availableFonts_.push_back(FontInfo{ "Default", io.Fonts->AddFontDefault() });
        printf("No custom fonts found. Default font loaded.\n");
    }
}

// Function to initialize fonts
void FontManager::initializeFonts(ImGuiIO& io, const std::string& fontsDir) {
    loadFontsFromDirectory(io, fontsDir);

    // Set the default font
    if (!availableFonts_.empty()) {
        io.FontDefault = availableFonts_[selectedFontIndex_].font;
    }
}

// Get available fonts
const std::vector<FontInfo>& FontManager::getAvailableFonts() const {
    return availableFonts_;
}

// Get selected font index
int FontManager::getSelectedFontIndex() const {
    return selectedFontIndex_;
}

// Set selected font index
void FontManager::setSelectedFontIndex(int index) {
    if (index >= 0 && index < static_cast<int>(availableFonts_.size())) {
        selectedFontIndex_ = index;
    }
}

// Get font scale
float FontManager::getFontScale() const {
    return fontScale_;
}

// Set font scale
void FontManager::setFontScale(float scale) {
    fontScale_ = scale;
}

// Get the selected font
ImFont* FontManager::getSelectedFont() const {
    if (selectedFontIndex_ >= 0 && selectedFontIndex_ < static_cast<int>(availableFonts_.size())) {
        return availableFonts_[selectedFontIndex_].font;
    }
    return nullptr;
}
