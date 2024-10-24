#include "map.hpp"
#include "stb_image.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include "notifications.hpp"

MapManager::MapManager(NotificationManager& notificationManager)
    : selectedMapIndex_(0), selectedConeIndex_(-1), showConeContextMenu_(false),
      notificationManager_(notificationManager) {}

// Destructor
MapManager::~MapManager() {
    // Cleanup map textures
    for (auto& map : maps_) {
        if (map.texture != 0) {
            GLuint texID = static_cast<GLuint>(reinterpret_cast<intptr_t>(map.texture));
            glDeleteTextures(1, &texID);
            map.texture = 0;
        }
    }
}

// Function to add a map
void MapManager::addMap(const std::string& name, const std::string& filePath, const ImVec2& boundBL, const ImVec2& boundTR) {
    maps_.push_back(MapInfo{ name, filePath, boundBL, boundTR, 0 });
    printf("Added map: %s\n", name.c_str());
}

// Helper function to load a JPG image
ImTextureID MapManager::loadImageJPG(const char *path)
{
    int width, height, channels;
    unsigned char *data = stbi_load(path, &width, &height, &channels, 4);
    if (data == NULL) {
        printf("Error loading image: %s\n", path);
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
    for (auto& map : maps_) {
        map.texture = loadImageJPG(map.filePath.c_str());
        if (map.texture == 0) {
            printf("Error loading map image: %s\n", map.filePath.c_str());
            notificationManager_.showPopup("MapManager_Error", "Error", "Unable to load map: " + map.name, NotificationType::Error);
        } else {
            printf("Map loaded successfully: %s\n", map.name.c_str());
        }
    }

    // Check if at least one map was loaded successfully
    bool anyMapLoaded = false;
    for (const auto& map : maps_) {
        if (map.texture != 0) {
            anyMapLoaded = true;
            break;
        }
    }

    if (!anyMapLoaded) {
        printf("Error: No map was loaded successfully.\n");
        notificationManager_.showPopup("MapManager_Error", "Error", "No map was loaded successfully.", NotificationType::Error);
        return false;  // Return false since no maps were loaded
    }
    return true;  // Return true since at least one map was loaded
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


// Get selected map index
int MapManager::getSelectedMapIndex() const {
    return selectedMapIndex_;
}

// Set selected map index
void MapManager::setSelectedMapIndex(int index) {
    if (index >= 0 && index < static_cast<int>(maps_.size())) {
        selectedMapIndex_ = index;
        notificationManager_.showPopup("MapManager", "Map Selected", "You have selected the map: " + maps_[index].name, NotificationType::Success);
        printf("Selected map: %s\n", maps_[index].name.c_str());
    }
}

// Get all maps
const std::vector<MapInfo>& MapManager::getMaps() const {
    return maps_;
}
