#include "file_browser.hpp"
#include "imgui.h"
#include "nfd.h"
#include "notifications.hpp"
#include "gps.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdio>
extern "C" {
    #include "gps_interface.h"
    #include "acr.h"
    #include "defines.h"
    #include "main.h"
    #include "utils.h"
}

bool showFileBrowser;
std::string currentPath;
std::vector<std::filesystem::directory_entry> entries; 
std::string selectedFile = "";


void initializeFileBrowser() {
    showFileBrowser = true; // Set to true to open automatically
    currentPath = "acr"; // Set initial directory to 'acr'
    entries.clear();
    selectedFile = "";
}

// Function to render the custom in-app file browser
void renderFileBrowser() {
    ImGui::Begin("File Browser", &showFileBrowser, ImGuiWindowFlags_AlwaysAutoResize);

    // Display current path
    ImGui::Text("Current Path: %s", currentPath.c_str());

    // Navigation buttons
    if (currentPath != std::filesystem::path(currentPath).root_path()) {
        if (ImGui::Button("..")) {
            currentPath = std::filesystem::path(currentPath).parent_path().string();
        }
        ImGui::SameLine();
    }

    // Refresh directory entries
    entries.clear();
    try {
        for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
            entries.emplace_back(entry);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Error accessing directory.");
    }

    // Sort entries: directories first, then files
    std::sort(entries.begin(), entries.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
        if (a.is_directory() && !b.is_directory()) return true;
        if (!a.is_directory() && b.is_directory()) return false;
        return a.path().filename().string() < b.path().filename().string();
    });

    // Display entries
    for (const auto& entry : entries) {
        const auto& path = entry.path();
        std::string name = path.filename().string();
        if (entry.is_directory()) {
            if (ImGui::Selectable((std::string("[") + name + "]").c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
                currentPath = path.string();
            }
        }
        else {
            if (ImGui::Selectable(name.c_str())) {
                // Only allow selection of log files (e.g., .log, .txt)
                if (path.extension() == ".log" || path.extension() == ".txt") {
                    selectedFile = path.string();
                }
            }
        }
    }

    // Load button
    if (!selectedFile.empty()) {
        if (ImGui::Button("Load")) {
            // Implement file loading logic here
            printf("Selected file: %s\n", selectedFile.c_str());

            // Stop existing GPS thread
            kill_thread.store(true);
            if (gpsThread.joinable()) {
                gpsThread.join();
            }

            // Close previous GPS interface
            gps_interface_close(&gps);

            // Open the selected log file
            if (gps_interface_open_file(&gps, selectedFile.c_str()) == -1) {
                printf("Failed to open selected file: %s\n", selectedFile.c_str());
                showPopup("FileBrowser", "Error", "Failed to open selected file.", NotificationType::Error);
            }
            else {
                {
                    std::unique_lock<std::mutex> lck(renderLock);
                    memset(&session, 0, sizeof(full_session_t));
                    memset(&cone_session, 0, sizeof(cone_session_t));
                    memset(&user_data, 0, sizeof(user_data_t));
                    user_data.basepath = currentPath.c_str();
                    user_data.cone = &cone;
                    user_data.session = &session;
                    user_data.cone_session = &cone_session;
                    trajectory.clear();
                    cones.clear();
                    currentPosition = ImVec2(0.0f, 0.0f); // Reset current GPS position
                }

                // Restart the GPS thread
                kill_thread.store(false);
                gpsThread = std::thread(readGPSLoop);

                // Show success notification
                showPopup("Success", "Success", "Successfully loaded log file.", NotificationType::Success);
            }

            selectedFile.clear();
            showFileBrowser = false;
        }

        // Tooltip for Load button
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to load the selected log file.");
        }
    }

    ImGui::End();
}