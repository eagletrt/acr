#include "map.hpp"
#include "icon_manager.hpp"
#include "stb_image.h"
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <iostream>
#include <thread>
#include <algorithm>


MapManager::MapManager(NotificationManager& notificationManager)
    : selectedConeIndex_(-1), showConeContextMenu_(false),
      selectedMapIndex_(0), notificationManager_(notificationManager) {}


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


void MapManager::addMap(const std::string& name, const std::string& filePath, const ImPlotPoint& boundBL, const ImPlotPoint& boundTR) {
    std::lock_guard<std::mutex> lock(mapMutex_);
    maps_.push_back(MapInfo{ name, filePath, boundBL, boundTR, 0 });
    std::cout << "Added map: " << name << std::endl;
}


void MapManager::loadMapsAsync() {
    for (size_t i = 1; i < maps_.size(); ++i) { 
        const MapInfo& map = maps_[i];
        std::vector<unsigned char> imageData;
        int width = 0, height = 0;

        if (loadImageData(map.filePath, imageData, width, height)) {
            
            {
                std::lock_guard<std::mutex> lock(pendingTexturesMutex_);
                pendingTextures_.push(PendingTexture{ map.name, std::move(imageData), width, height });
            }
            std::cout << "Map data loaded for: " << map.name << std::endl;
            
            notificationManager_.showPopup("MapManager", "Loading", "Loading map: " + map.name, NotificationType::Info);
        }
        else {
            std::cerr << "Error loading image data for map: " << map.name << std::endl;
            notificationManager_.showPopup("MapManager_Error", "Error", "Unable to load map: " + map.name, NotificationType::Error);
        }
    }
}


bool MapManager::loadImageData(const std::string& filePath, std::vector<unsigned char>& imageData, int& width, int& height) {
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, nullptr, 4); 
    if (data) {
        imageData.assign(data, data + (width * height * 4));
        stbi_image_free(data);
        return true;
    }
    else {
        std::cerr << "Failed to load image: " << filePath << std::endl;
        return false;
    }
}


bool MapManager::loadMapTextures() {
    std::lock_guard<std::mutex> lock(mapMutex_);
    if (maps_.empty()) {
        std::cerr << "No maps to load." << std::endl;
        return false;
    }

    
    MapInfo& firstMap = maps_[0];
    std::vector<unsigned char> imageData;
    int width = 0, height = 0;
    if (!loadImageData(firstMap.filePath, imageData, width, height)) {
        std::cerr << "Failed to load first map image: " << firstMap.filePath << std::endl;
        notificationManager_.showPopup("MapManager_Error", "Error", "Unable to load map: " + firstMap.name, NotificationType::Error);
        return false;
    }

    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());

    firstMap.texture = (ImTextureID)(intptr_t)tex;

    std::cout << "First map loaded successfully: " << firstMap.name << std::endl;
    notificationManager_.showPopup("MapManager", "Success", "Loaded map: " + firstMap.name, NotificationType::Success);

    
    std::thread asyncLoader(&MapManager::loadMapsAsync, this);
    asyncLoader.detach();

    return true;
}

void MapManager::processPendingTextures() {
    std::lock_guard<std::mutex> lockPending(pendingTexturesMutex_);
    std::lock_guard<std::mutex> lockMap(mapMutex_);
    while (!pendingTextures_.empty()) {
        PendingTexture pending = std::move(pendingTextures_.front());
        pendingTextures_.pop();

        
        auto it = std::find_if(maps_.begin(), maps_.end(), [&](const MapInfo& m) {
            return m.name == pending.name;
        });

        if (it != maps_.end()) {
            
            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);

            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pending.width, pending.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pending.imageData.data());

            it->texture = (ImTextureID)(intptr_t)tex;

            std::cout << "Map texture created for: " << it->name << std::endl;
            notificationManager_.showPopup("MapManager", "Success", "Loaded map: " + it->name, NotificationType::Success);
        } else {
            std::cerr << "Map not found for texture creation: " << pending.name << std::endl;
            notificationManager_.showPopup("MapManager_Error", "Error", "Map not found: " + pending.name, NotificationType::Error);
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
