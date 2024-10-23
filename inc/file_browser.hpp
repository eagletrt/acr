// file_browser.hpp
#ifndef FILE_BROWSER_HPP
#define FILE_BROWSER_HPP

#include <string>
#include <vector>
#include <filesystem>
#include "imgui.h" 

extern bool showFileBrowser; // Indicates if the file browser should be shown
extern std::string currentPath; // Current directory path
extern std::vector<std::filesystem::directory_entry> entries; // Directory entries
extern std::string selectedFile; // Currently selected file

// Function to render the file browser UI
void renderFileBrowser();

// Function to initialize file browser state
void initializeFileBrowser();

#endif // FILE_BROWSER_HPP