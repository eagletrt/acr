
#include "map.hpp"
#include "stb_image.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <thread>
#include <mutex>
#include "notifications.hpp"
#include "config.hpp"
#include <iostream>

MapManager::MapManager(NotificationManager& notificationManager)
    : selectedMapIndex_(0), selectedConeIndex_(-1), showConeContextMenu_(false),
      notificationManager_(notificationManager) {}

MapManager::~MapManager() {
    
    std::lock_guard<std::mutex> lock(mapMutex_);
    for (auto& map : maps_) {
        if (map.texture != 0) {
            GLuint texID = static_cast<GLuint>(reinterpret_cast<intptr_t>(map.texture));
            glDeleteTextures(1, &texID);
            map.texture = 0;
        }
    }
}

void MapManager::addMap(const std::string& name, const std::string& filePath, const ImVec2& boundBL, const ImVec2& boundTR) {
    std::lock_guard<std::mutex> lock(mapMutex_);
    maps_.push_back(MapInfo{ name, filePath, boundBL, boundTR, 0 });
    std::cout << "Added map: " << name << std::endl;
}

ImTextureID MapManager::loadImageJPG(const char *path)
{
    int width, height, channels;
    unsigned char *data = stbi_load(path, &width, &height, &channels, 4);
    if (data == NULL) {
        std::cerr << "Error loading image: " << path << std::endl;
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return (ImTextureID)(intptr_t)tex;
}

bool MapManager::loadMapTextures() {
    
    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        if (!maps_.empty()) {
            MapInfo& firstMap = maps_[0];
            firstMap.texture = loadImageJPG(firstMap.filePath.c_str());
            if (firstMap.texture == 0) {
                std::cerr << "Error loading first map image: " << firstMap.filePath << std::endl;
                notificationManager_.showPopup("MapManager_Error", "Error", "Unable to load first map: " + firstMap.name, NotificationType::Error);
                
            }
            else {
                std::cout << "First map loaded successfully: " << firstMap.name << std::endl;
            }
        }
        else {
            std::cerr << "No maps to load." << std::endl;
            return false;
        }
    }

    
    std::thread asyncMapLoader([this]() {
        for (size_t i = 1; i < maps_.size(); ++i) { 
            ImTextureID tex = loadImageJPG(maps_[i].filePath.c_str());
            {
                std::lock_guard<std::mutex> lock(mapMutex_);
                maps_[i].texture = tex;
            }
            if (tex == 0) {
                std::cerr << "Error loading map image asynchronously: " << maps_[i].filePath << std::endl;
                notificationManager_.showPopup("MapManager_Error", "Error", "Unable to load map: " + maps_[i].name, NotificationType::Error);
            }
            else {
                std::cout << "Map loaded asynchronously: " << maps_[i].name << std::endl;
                notificationManager_.showPopup("MapManager", "Map Loaded", "Map loaded: " + maps_[i].name, NotificationType::Success);
            }
        }
    });
    asyncMapLoader.detach(); 

    
    {
        std::lock_guard<std::mutex> lock(mapMutex_);
        if (!maps_.empty() && maps_[0].texture != 0) {
            return true;
        }
        else {
            std::cerr << "Error: First map was not loaded successfully." << std::endl;
            return false;
        }
    }
}

int MapManager::findClosestCone(const ImPlotPoint& mousePos, const std::vector<cone_t>& cones, float hitRadius) const {
    int closestConeIndex = -1;
    float minDistSq = hitRadius * hitRadius;

    for (size_t i = 0; i < cones.size(); ++i) {
        float dx = static_cast<float>(cones[i].lon - mousePos.x);
        float dy = static_cast<float>(cones[i].lat - mousePos.y);
        float distSq = dx * dx + dy * dy;
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestConeIndex = static_cast<int>(i);
        }
    }
    return closestConeIndex;
}

int MapManager::getSelectedMapIndex() const {
    return selectedMapIndex_;
}

void MapManager::setSelectedMapIndex(int index) {
    std::lock_guard<std::mutex> lock(mapMutex_);
    if (index >= 0 && index < static_cast<int>(maps_.size())) {
        selectedMapIndex_ = index;
        notificationManager_.showPopup("MapManager", "Map Selected", "You have selected the map: " + maps_[index].name, NotificationType::Success);
        std::cout << "Selected map: " << maps_[index].name << std::endl;
    }
}

const std::vector<MapInfo>& MapManager::getMaps() const {
    return maps_;
}
