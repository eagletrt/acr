// cones_loader.hpp
#ifndef CONES_LOADER_HPP
#define CONES_LOADER_HPP

#include <string>
#include <vector>
#include <mutex>
#include "map.hpp"  

// Function to load cones from a CSV file
bool loadConesFromCSV(const std::string& filePath);

// Optional: Function to clear all loaded cones
void clearLoadedCones();

#endif // CONES_LOADER_HPP