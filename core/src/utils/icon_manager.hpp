
#ifndef ICON_MANAGER_HPP
#define ICON_MANAGER_HPP

#include <string>
#include <vector>
#include "imgui.h" 
#include "config.hpp"


class NotificationManager;


struct IconInfo {
    std::string name;
    ImTextureID texture;
    ImVec2 size;
};

class IconManager {
public:
    IconManager();
    ~IconManager();

    
    void loadIcons(NotificationManager& notificationManager);

    
    ImTextureID getIconTexture(const std::string& name) const;

private:
    std::vector<IconInfo> icons_;

    
    ImTextureID loadIconImage(const char* path, NotificationManager& notificationManager);
};

#endif 
