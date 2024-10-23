#include "font_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include "notifications.hpp"

// Definition of global variables
std::vector<FontInfo> availableFonts;
int selectedFontIndex = 0; // Index of the selected font
float fontScale = 1.0f; // Initial font scale

// Function to load fonts from the directory
void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir) {
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
                    availableFonts.push_back(FontInfo{ name, font });
                    printf("Font loaded: %s\n", name.c_str());
                }
                else {
                    printf("Unable to load font: %s\n", path.c_str());
                }
            }
        }
    }

    // If no custom fonts were loaded, load the default font
    if (availableFonts.empty()) {
        availableFonts.push_back(FontInfo{ "Default", io.Fonts->AddFontDefault() });
        printf("No custom fonts found. Default font loaded.\n");
    }
}

// Function to initialize fonts
void initializeFonts(ImGuiIO& io, const std::string& fontsDir) {
    loadFontsFromDirectory(io, fontsDir);

    // Set the default font
    if (!availableFonts.empty()) {
        io.FontDefault = availableFonts[selectedFontIndex].font;
    }
}
