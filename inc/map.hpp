#ifndef MAP_HPP
#define MAP_HPP

#include <string>
#include <vector>
#include "imgui.h"
#include "implot.h"
#include "notifications.hpp" 

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

class MapManager {
public:
    MapManager(NotificationManager& notificationManager);
    ~MapManager();

    // Add a map
    void addMap(const std::string& name, const std::string& filePath, const ImVec2& boundBL, const ImVec2& boundTR);

    // Load map textures
    bool loadMapTextures();

    // Find the index of the closest cone to a point on the map
    int findClosestCone(const ImPlotPoint& mousePos, const std::vector<cone_t>& cones, float hitRadius) const;

    // Getters and setters for selected map
    int getSelectedMapIndex() const;
    void setSelectedMapIndex(int index);

    // Get maps
    const std::vector<MapInfo>& getMaps() const;

    // Selected cone index and context menu flag (made public for simplicity)
    int selectedConeIndex_;
    bool showConeContextMenu_;
    int selectedMapIndex_=0;

private:
    std::vector<MapInfo> maps_;
    

    NotificationManager& notificationManager_;

    // Helper function to load a JPG image
    ImTextureID loadImageJPG(const char *path);
};

#endif // MAP_HPP
