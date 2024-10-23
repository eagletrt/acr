#ifndef ICON_MANAGER_HPP
#define ICON_MANAGER_HPP

#include <string>
#include <vector>
#include "imgui.h" 

// Structure for icon information
struct IconInfo {
    std::string name;
    ImTextureID texture;
    ImVec2 size;
};

// Function to load icons
void loadIcons();

// Function to get an icon texture by name
ImTextureID getIconTexture(const std::string& name);

// Externally accessible icons vector
extern std::vector<IconInfo> icons;

#endif