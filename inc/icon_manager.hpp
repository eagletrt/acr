#ifndef ICON_MANAGER_HPP
#define ICON_MANAGER_HPP

#include <string>
#include <vector>
#include "imgui.h" 
#include "notifications.hpp"

// Structure for icon information
struct IconInfo {
    std::string name;
    ImTextureID texture;
    ImVec2 size;
};

class IconManager {
public:
    IconManager();
    ~IconManager();

    // Load icons
    void loadIcons(NotificationManager& notificationManager);

    // Get icon texture by name
    ImTextureID getIconTexture(const std::string& name) const;
    

private:
    std::vector<IconInfo> icons_;

    // Helper function to load a PNG image
    ImTextureID loadImagePNG(const char* path, NotificationManager& notificationManager);
};

#endif // ICON_MANAGER_HPP
