#ifndef MAP_HPP
#define MAP_HPP

#include <string>
#include <vector>
#include "imgui.h" 
#include "implot.h" 

// Include the header where 'cone_t' is defined
extern "C" {
    #include "defines.h"
    #include "acr.h"
}

// Structure for map information
struct MapInfo {
    std::string name;      // Name of the map
    std::string filePath;  // Path to the image file
    ImVec2 boundBL;        // Bottom-left boundary coordinates
    ImVec2 boundTR;        // Top-right boundary coordinates
    ImTextureID texture;   // Loaded texture ID
};

// Function to add a map
void addMap(const std::string& name, const std::string& filePath, const ImVec2& boundBL, const ImVec2& boundTR);

// Function to load map textures
void loadMapTextures();

// Function to find the index of the closest cone to a point on the map
int findClosestCone(const ImPlotPoint& mousePos, const std::vector<cone_t>& cones, float hitRadius);

// Externally accessible maps vector
extern std::vector<MapInfo> maps;

// External variable for selected map
extern int selectedMapIndex;

// Externally accessible variables for cone selection
extern int selectedConeIndex;
extern bool showConeContextMenu;

#endif