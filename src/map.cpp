#include "map.hpp"
#include "stb_image.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include "notifications.hpp"
#include "icon_manager.hpp"

extern "C" {
    #include "defines.h"
    #include "acr.h"
}

// Define external variables
std::vector<MapInfo> maps;
int selectedMapIndex = 0; // Index of the selected map

int selectedConeIndex = -1;
bool showConeContextMenu = false;

// Function to add a map
void addMap(const std::string& name, const std::string& filePath, const ImVec2& boundBL, const ImVec2& boundTR) {
    maps.push_back(MapInfo{ name, filePath, boundBL, boundTR, 0 });
}

ImTextureID loadImageJPG(const char *path)
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

void loadMapTextures() {
    for (auto& map : maps) {
        map.texture = loadImageJPG(map.filePath.c_str());
        if (map.texture == 0) {
            printf("Error loading map image: %s\n", map.filePath.c_str());
            showPopup("Map_Error", "Error", "Unable to load map: " + map.name, NotificationType::Error);
        }
        else {
            printf("Map loaded successfully: %s\n", map.name.c_str());
        }
    }

    // Check if at least one map was loaded successfully
    bool anyMapLoaded = false;
    for (const auto& map : maps) {
        if (map.texture != 0) {
            anyMapLoaded = true;
            break;
        }
    }
    if (!anyMapLoaded) {
        printf("Error: No map was loaded successfully.\n");
        showPopup("Map_Error", "Error", "No map was loaded successfully.", NotificationType::Error);
    }
}

// Function to find the index of the closest cone to a point on the map
int findClosestCone(const ImPlotPoint& mousePos, const std::vector<cone_t>& cones, float hitRadius) {
    int closestIndex = -1;
    float closestDistance = hitRadius;  // Consider the cone only if it's within this radius

    for (size_t i = 0; i < cones.size(); ++i) {
        float distance = sqrtf(powf(mousePos.x - cones[i].lon, 2) + powf(mousePos.y - cones[i].lat, 2));
        if (distance < closestDistance) {
            closestIndex = static_cast<int>(i);
            closestDistance = distance;
        }
    }

    return closestIndex; 
}