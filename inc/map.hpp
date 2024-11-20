
#ifndef MAP_HPP
#define MAP_HPP

#include <string>
#include <vector>
#include <mutex> 
#include "imgui.h"
#include "implot.h"
#include "notifications.hpp" 
#include "config.hpp"

extern "C" {
    #include "defines.h"
    #include "acr.h"
}


struct MapInfo {
    std::string name;     
    std::string filePath; 
    ImVec2 boundBL;       
    ImVec2 boundTR;       
    ImTextureID texture;  
};

class NotificationManager;

class MapManager {
public:
    MapManager(NotificationManager& notificationManager);
    ~MapManager();

    
    void addMap(const std::string& name, const std::string& filePath, const ImVec2& boundBL, const ImVec2& boundTR);

    
    bool loadMapTextures();

    
    int findClosestCone(const ImPlotPoint& mousePos, const std::vector<cone_t>& cones, float hitRadius) const;

    
    int getSelectedMapIndex() const;
    void setSelectedMapIndex(int index);

    
    const std::vector<MapInfo>& getMaps() const;

    
    int selectedConeIndex_;
    bool showConeContextMenu_;
    int selectedMapIndex_;
     mutable std::mutex mapMutex_; 

private:
    std::vector<MapInfo> maps_;          
    NotificationManager& notificationManager_; 
          

    
    ImTextureID loadImageJPG(const char *path);
};

#endif 
