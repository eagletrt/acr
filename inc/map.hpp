#ifndef MAP_HPP
#define MAP_HPP

#include <string>
#include <vector>
#include <mutex>
#include <queue>
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
    ImPlotPoint boundBL;            
    ImPlotPoint boundTR;            
    ImTextureID texture;       
};


struct PendingTexture {
    std::string name;                      
    std::vector<unsigned char> imageData;  
    int width;                             
    int height;                            
};

class NotificationManager;


class MapManager {
public:
    MapManager(NotificationManager& notificationManager);
    ~MapManager();

    
    void addMap(const std::string& name, const std::string& filePath, const ImPlotPoint& boundBL, const ImPlotPoint& boundTR);

    
    bool loadMapTextures();

    
    void processPendingTextures();

    
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

    
    std::queue<PendingTexture> pendingTextures_;
    std::mutex pendingTexturesMutex_;

    
    void loadMapsAsync();

    
    bool loadImageData(const std::string& filePath, std::vector<unsigned char>& imageData, int& width, int& height);
};

#endif
