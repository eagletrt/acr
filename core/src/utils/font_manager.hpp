
#ifndef FONT_MANAGER_HPP
#define FONT_MANAGER_HPP

#include <string>
#include <vector>
#include <mutex>       
#include "imgui.h"
#include "config.hpp"


struct FontInfo {
    std::string name;
    ImFont* font;
};

class FontManager {
public:
    FontManager();
    ~FontManager();

    
    void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir, int lastFontIndex);

    
    void initializeFonts(ImGuiIO& io, const std::string& fontsDir, int lastFontIndex);

    
    const std::vector<FontInfo>& getAvailableFonts() const;

    
    int getSelectedFontIndex() const;
    void setSelectedFontIndex(int index);

    
    float getFontScale() const;
    void setFontScale(float scale);

    
    ImFont* getSelectedFont() const;
    mutable std::mutex fontsMutex_; 
    float fontScale_;  

private:
    std::vector<FontInfo> availableFonts_; 
    int selectedFontIndex_;                
                        

                   
};

#endif 
