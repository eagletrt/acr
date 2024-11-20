
#include "font_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iostream>
#include "notifications.hpp"
#include "config.hpp"

FontManager::FontManager()
    : selectedFontIndex_(0), fontScale_(1.0f) {}

FontManager::~FontManager() {}

void FontManager::loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir, int lastFontIndex) {
    
    std::vector<std::string> extensions = { ".ttf", ".otf" };

    
    if (lastFontIndex < 0) lastFontIndex = 0;
    std::vector<std::filesystem::directory_entry> fontFiles;

    for (const auto& entry : std::filesystem::directory_iterator(fontsDir)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();
            
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                fontFiles.emplace_back(entry);
            }
        }
    }

    if (fontFiles.empty()) {
        std::cerr << "No font files found in directory: " << fontsDir << std::endl;
    }

    
    std::sort(fontFiles.begin(), fontFiles.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
        return a.path().filename().string() < b.path().filename().string();
    });

    
    if (lastFontIndex < static_cast<int>(fontFiles.size())) {
        const auto& entry = fontFiles[lastFontIndex];
        std::string path = entry.path().string();
        std::string name = entry.path().stem().string();
        ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
        if (font) {
            {
                std::lock_guard<std::mutex> lock(fontsMutex_);
                availableFonts_.push_back(FontInfo{ name, font });
            }
            std::cout << "Font loaded: " << name << std::endl;
            selectedFontIndex_ = lastFontIndex;
        }
        else {
            std::cerr << "Unable to load font: " << path << std::endl;
        }
    }
    else {
        
        if (!fontFiles.empty()) {
            const auto& entry = fontFiles[0];
            std::string path = entry.path().string();
            std::string name = entry.path().stem().string();
            ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
            if (font) {
                {
                    std::lock_guard<std::mutex> lock(fontsMutex_);
                    availableFonts_.push_back(FontInfo{ name, font });
                }
                std::cout << "Font loaded: " << name << std::endl;
                selectedFontIndex_ = 0;
            }
            else {
                std::cerr << "Unable to load font: " << path << std::endl;
            }
        }
    }

    
    std::thread asyncFontLoader([this, &io, fontsDir, fontFiles, lastFontIndex]() {
        for (size_t i = 0; i < fontFiles.size(); ++i) {
            if (static_cast<int>(i) == lastFontIndex) continue; 
            const auto& entry = fontFiles[i];
            std::string path = entry.path().string();
            std::string name = entry.path().stem().string();
            ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
            if (font) {
                {
                    std::lock_guard<std::mutex> lock(fontsMutex_);
                    availableFonts_.push_back(FontInfo{ name, font });
                }
                std::cout << "Font loaded asynchronously: " << name << std::endl;
            }
            else {
                std::cerr << "Unable to load font asynchronously: " << path << std::endl;
            }
        }

        
        io.Fonts->Build();
        std::cout << "Asynchronous font loading completed. Font atlas rebuilt." << std::endl;
    });
    asyncFontLoader.detach(); 
}

void FontManager::initializeFonts(ImGuiIO& io, const std::string& fontsDir, int lastFontIndex) {
    loadFontsFromDirectory(io, fontsDir, lastFontIndex);

    
    {
        std::lock_guard<std::mutex> lock(fontsMutex_);
        if (!availableFonts_.empty() && selectedFontIndex_ < static_cast<int>(availableFonts_.size())) {
            io.FontDefault = availableFonts_[selectedFontIndex_].font;
            std::cout << "Default font set to: " << availableFonts_[selectedFontIndex_].name << std::endl;
        }
        else {
            
            io.FontDefault = io.Fonts->Fonts.empty() ? nullptr : io.Fonts->Fonts[0];
            if (io.FontDefault) {
                std::cout << "Fallback to ImGui's default font." << std::endl;
            }
            else {
                std::cerr << "No fonts loaded. io.FontDefault is nullptr." << std::endl;
            }
        }
    }
}

const std::vector<FontInfo>& FontManager::getAvailableFonts() const {
    return availableFonts_;
}

int FontManager::getSelectedFontIndex() const {
    return selectedFontIndex_;
}

void FontManager::setSelectedFontIndex(int index) {
    std::lock_guard<std::mutex> lock(fontsMutex_);
    if (index >= 0 && index < static_cast<int>(availableFonts_.size())) {
        selectedFontIndex_ = index;
    }
}

float FontManager::getFontScale() const {
    return fontScale_;
}

void FontManager::setFontScale(float scale) {
    fontScale_ = scale;
}

ImFont* FontManager::getSelectedFont() const {
    std::lock_guard<std::mutex> lock(fontsMutex_);
    if (selectedFontIndex_ >= 0 && selectedFontIndex_ < static_cast<int>(availableFonts_.size())) {
        return availableFonts_[selectedFontIndex_].font;
    }
    return nullptr;
}
