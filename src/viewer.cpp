#define STB_IMAGE_IMPLEMENTATION
#include "viewer.hpp"
#include <string.h>
#include <atomic>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <queue> 
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>  
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "stb_image.h"
#include "nfd.h"

extern "C" {
#include "acr.h"
#include "defines.h"
#include "main.h"
#include "utils.h"
}

#include "gui.hpp"
#include "gps.hpp"
#include "file_browser.hpp"
#include "notifications.hpp"
#include "map.hpp"
#include "font_manager.hpp"
#include "icon_manager.hpp"
#include "utils.hpp"
#include "cones_loader.hpp"

int main(int argc, char **argv) {
    // Initialize Native File Dialog
    NFD_Init();

    // Set initial directory to Desktop
    std::string desktopPath = getDesktopPath();
    currentPath = desktopPath;

    const char *basepath = getenv("HOME");

    // Setup ImGui and create a GLFW window
    GLFWwindow *window = setupImGui();
    if (window == nullptr) {
        printf("Failed to initialize GLFW or create window.\n");
        return -1;
    }

    // Load icons
    loadIcons();

    // Initialize session data structures
    memset(&session, 0, sizeof(full_session_t));
    memset(&cone_session, 0, sizeof(cone_session_t));
    memset(&user_data, 0, sizeof(user_data_t));
    user_data.basepath = currentPath.c_str(); 
    user_data.cone = &cone;
    user_data.session = &session;
    user_data.cone_session = &cone_session;

    // Initialize GPS thread
    gpsThread = std::thread(); 

    // Add maps with their respective boundaries
    addMap("Povo", "../assets/Povo.jpg", ImVec2{11.1484815430000008, 46.0658863580000002}, ImVec2{11.1515535430000003, 46.0689583580000033});
    addMap("Vadena", "../assets/Vadena.jpg", ImVec2{11.3097566090000008, 46.4300119620000018}, ImVec2{11.3169246090000009, 46.4382039620000029});
    addMap("FSG", "../assets/FSG.jpg", ImVec2{8.558931763076039, 49.32344089057215}, ImVec2{8.595726757113576, 49.335430701657735});
    addMap("Ala", "../assets/Ala.jpg", ImVec2{11.010747212958533, 45.784567764275764}, ImVec2{11.013506837511347, 45.78713363420464});
    addMap("Varano", "../assets/Varano.jpg", ImVec2{10.013347232876077, 44.67756187921092}, ImVec2{10.031744729894845, 44.684102509035036});

    // Load textures for all maps
    loadMapTextures();

    float mapOpacity = 0.5f;
    float lastTime = glfwGetTime();
    bool resetView = false;
    bool isFirstRun = true;

    // Check for serial ports /dev/ttyACM0 and /dev/ttyACM1
    const char* serialPorts[] = { "/dev/ttyACM0", "/dev/ttyACM1" };
    const char* detectedPort = nullptr;

    for (const char* port : serialPorts) {
        struct stat statbuf;
        if (stat(port, &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
            // Serial port exists
            detectedPort = port;
            break;
        }
    }

    if (detectedPort) {
        printf("Detected serial port: %s\n", detectedPort);
        // Change permissions to allow reading
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "sudo chmod 666 %s", detectedPort);
        system(cmd);
        // Initialize GPS with the detected serial port
        if (initializeGPS(detectedPort) == -1) {
            printf("Error: GPS not found or failed to initialize on port %s.\n", detectedPort);
            showPopup("GPS_Error", "Error", "GPS not found or failed to initialize.", NotificationType::Error);
        } else {
            // Start the GPS reading thread
            kill_thread.store(false);
            gpsThread = std::thread(readGPSLoop);
            printf("Started reading GPS data from serial port %s.\n", detectedPort);
            showPopup("GPS", "GPS Connected", "Reading GPS data from serial port.", NotificationType::Success);
        }
    } else {
        printf("No serial port detected. Proceeding to file selection.\n");
    }

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        startFrame();

        // Process active notifications
        processNotifications(deltaTime);

        // Get the main viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportPos = viewport->Pos;
        ImVec2 viewportSize = viewport->Size;

        // Set the next window position and size to cover the entire viewport
        ImGui::SetNextWindowPos(viewportPos);
        ImGui::SetNextWindowSize(viewportSize);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;

        // Begin the fullscreen window "ACR"
        if (ImGui::Begin("ACR", nullptr, window_flags)) {
            
            if (isFirstRun) {
                isFirstRun = false;
                nfdu8filteritem_t logFilterList[] = {
                    { "Log files", "log,txt" } 
                };
                size_t logFilterCount = sizeof(logFilterList) / sizeof(logFilterList[0]);

                // Initialize dialog arguments
                nfdopendialogu8args_t logArgs = {0};
                logArgs.filterList = logFilterList;
                logArgs.filterCount = logFilterCount;
                logArgs.defaultPath = NULL; 

                nfdu8char_t* logOutPath = nullptr;

                // Open file dialog to select the log file
                nfdresult_t logResult = NFD_OpenDialogU8_With(&logOutPath, &logArgs);

                if (logResult == NFD_OKAY && logOutPath != nullptr) {
                    printf("Selected log file: %s\n", logOutPath);
                    kill_thread.store(true);
                    if (gpsThread.joinable()) {
                        gpsThread.join();
                    }

                    gps_interface_close(&gps);

                    if (gps_interface_open_file(&gps, logOutPath) == -1) {
                        printf("Error opening selected file: %s\n", logOutPath);
                        showPopup("FileBrowser", "Error", "Failed to open selected file.", NotificationType::Error);
                    } else {
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
                            currentPosition = ImVec2(0.0f, 0.0f); 
                        }

                        kill_thread.store(false);
                        gpsThread = std::thread(readGPSLoop);

                        showPopup("Success", "Success", "Successfully loaded log file.", NotificationType::Success);
                    }

                    NFD_FreePathU8(logOutPath);
                }
                else if (logResult == NFD_CANCEL) {
                    printf("User canceled log file selection.\n");
                    glfwSetWindowShouldClose(window, false);
                }
                else {
                    printf("Error selecting log file: %s\n", NFD_GetError());
                    glfwSetWindowShouldClose(window, true);
                }
            }

            // Button to load a new log file
            if (ImGui::Button("Load Log File")) {
                nfdu8filteritem_t filterList[] = {
                    { "Log files", "log,txt" } 
                };
                size_t filterCount = sizeof(filterList) / sizeof(filterList[0]);
                nfdopendialogu8args_t args = {0};
                args.filterList = filterList;
                args.filterCount = filterCount;
                args.defaultPath = NULL; 

                nfdu8char_t* outPath = nullptr;

                // Open file dialog to select the log file
                nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

                if (result == NFD_OKAY && outPath != nullptr) {
                    printf("Selected file: %s\n", outPath);

                    // Stop existing GPS thread if active
                    kill_thread.store(true);
                    if (gpsThread.joinable()) {
                        gpsThread.join();
                    }

                    // Close previous GPS interface
                    gps_interface_close(&gps);

                    // Open the selected log file
                    if (gps_interface_open_file(&gps, outPath) == -1) {
                        printf("Error opening selected file: %s\n", outPath);
                        showPopup("FileBrowser", "Error", "Failed to open selected file.", NotificationType::Error);
                    } else {
                        // Reset session data and clear previous data
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

                    // Free the path returned by NFD
                    NFD_FreePathU8(outPath);
                }
                else if (result == NFD_CANCEL) {
                    printf("User canceled file selection.\n");
                }
                else {
                    printf("Error: %s\n", NFD_GetError());
                }
            }

            // Button to load the cones CSV file
            if (ImGui::Button("Load Cones CSV")) {
                // Define filters for CSV files
                nfdu8filteritem_t filterList[] = {
                    { "CSV files", "csv" }  
                };
                size_t filterCount = sizeof(filterList) / sizeof(filterList[0]);

                // Initialize dialog arguments
                nfdopendialogu8args_t args = {0};
                args.filterList = filterList;
                args.filterCount = filterCount;
                args.defaultPath = NULL; 

                // Declare outPath
                nfdu8char_t* outPath = nullptr;

                // Open file dialog to select the CSV file
                nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

                if (result == NFD_OKAY && outPath != nullptr) {
                    printf("Selected CSV file: %s\n", outPath);

                    // Load cones from the selected CSV file
                    if (loadConesFromCSV(outPath)) {
                        showPopup("Cones_Loaded", "Success", "Successfully loaded cones from CSV.", NotificationType::Success);
                        printf("Cones loaded from CSV: %s\n", outPath);
                        resetView = true; // Set resetView to update map limits
                    } else {
                        showPopup("Cones_Load_Error", "Error", "Failed to load cones from the selected CSV file.", NotificationType::Error);
                        printf("Failed to load cones from CSV: %s\n", outPath);
                    }

                    // Free the path returned by NFD
                    NFD_FreePathU8(outPath);
                }
                else if (result == NFD_CANCEL) {
                    printf("User canceled CSV file selection.\n");
                }
                else {
                    printf("Error selecting CSV file: %s\n", NFD_GetError());
                }
            }

            // Settings section using Tab Bar
            if (ImGui::BeginTabBar("MainTabBar")) {
                if (ImGui::BeginTabItem("Help")) {
                    ImGui::Text("Quit (Q)");
                    ImGui::Text("Toggle Trajectory Recording (T)");
                    ImGui::Text("Cones: ");
                    ImGui::Text("- Orange (O)");
                    ImGui::Text("- Yellow (Y)");
                    ImGui::Text("- Blue (B)");
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings")) {
                    // Slider for map opacity with tooltip
                    ImGui::BeginGroup();
                    ImGui::Text("Map Opacity");
                    ImGui::SameLine();
                    ImGui::SliderFloat("##MapOpacity", &mapOpacity, 0.0f, 1.0f);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Adjust the opacity of the map overlay.");
                    ImGui::EndGroup();

                    // Radio buttons for map selection with tooltip
                    ImGui::Text("Select Map:");
                    for (int i = 0; i < static_cast<int>(maps.size()); i++) {
                        if (ImGui::RadioButton(maps[i].name.c_str(), &selectedMapIndex, i)) {
                            // Show a confirmation popup when a map is selected
                            showPopup("Map", "Map Selected", "You have selected the map: " + maps[i].name, NotificationType::Success);
                            printf("Selected map: %s\n", maps[i].name.c_str());

                            // Reset view when a new map is selected
                            resetView = true;
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Select the map to display.");
                    }

                    // Combo box for font selection with tooltip
                    if (ImGui::BeginCombo("Select Font", availableFonts[selectedFontIndex].name.c_str())) {
                        for (size_t n = 0; n < availableFonts.size(); n++) {
                            bool is_selected = (selectedFontIndex == static_cast<int>(n));
                            if (ImGui::Selectable(availableFonts[n].name.c_str(), is_selected))
                                selectedFontIndex = static_cast<int>(n);
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Choose a font for the application.");

                    // Set the selected font
                    if (selectedFontIndex >= 0 && selectedFontIndex < static_cast<int>(availableFonts.size())) {
                        ImGui::GetIO().FontDefault = availableFonts[selectedFontIndex].font;
                    }

                    // Slider for font scale with tooltip
                    ImGui::BeginGroup();
                    ImGui::Text("Font Scale");
                    ImGui::SameLine();
                    ImGui::SliderFloat("##FontScale", &fontScale, 0.5f, 2.0f);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Adjust the global font scale.");
                    ImGui::EndGroup();

                    ImGui::Text("Font Scale: %.2f", fontScale);
                    ImGui::GetIO().FontGlobalScale = fontScale;

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::Separator();

            // Display HDOP with tooltip
            ImGui::Text("HDOP: %.2f [m]", gps_data.hpposllh.hAcc);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Horizontal Dilution of Precision (HDOP) indicates the GPS accuracy.");

            {
                std::unique_lock<std::mutex> lck(renderLock);
                // Handle key presses with tooltips
                if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                    if (session.active) {
                        csv_session_stop(&session);
                        printf("Session '%s' ended.\n", session.session_name);
                        showPopup("Session", "Session Stopped", "Recording session ended.", NotificationType::Info);
                    }
                    else {
                        if (csv_session_setup(&session, basepath) == -1) {
                            printf("Error: Session setup failed.\n");
                            showPopup("Session", "Error", "Session setup failed.", NotificationType::Error);
                        }
                        if (csv_session_start(&session) == -1) {
                            printf("Error: Session start failed.\n");
                            showPopup("Session", "Error", "Session start failed.", NotificationType::Error);
                        }
                        printf("Session '%s' started [%s].\n", session.session_name,
                               session.session_path);
                        showPopup("Session", "Session Started", "Recording session started.", NotificationType::Info);
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_O)) {
                    cone.id = CONE_ID_ORANGE;
                    save_cone.store(true);
                    showPopup("Cone_Orange", "Cone Placed", "An orange cone has been placed.", NotificationType::Success);
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
                    cone.id = CONE_ID_YELLOW;
                    save_cone.store(true);
                    showPopup("Cone_Yellow", "Cone Placed", "A yellow cone has been placed.", NotificationType::Success);
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_B)) {
                    cone.id = CONE_ID_BLUE;
                    save_cone.store(true);
                    showPopup("Cone_Blue", "Cone Placed", "A blue cone has been placed.", NotificationType::Success);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
                    glfwSetWindowShouldClose(window, true);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_C)) {
                    trajectory.clear();
                    cones.clear();
                    printf("Trajectory and cones cleared.\n");
                    showPopup("Cleared", "Cleared", "Trajectory and cones have been cleared.", NotificationType::Info);
                }
                if (save_cone.load() && cone_session.active == 0) {
                    if (cone_session_setup(&cone_session, basepath) == -1) {
                        printf("Error: Cone session setup failed.\n");
                        showPopup("Cone_Session", "Error", "Cone session setup failed.", NotificationType::Error);
                    }
                    if (cone_session_start(&cone_session) == -1) {
                        printf("Error: Cone session start failed.\n");
                        showPopup("Cone_Session", "Error", "Cone session start failed.", NotificationType::Error);
                    }
                    printf("Cone session '%s' started [%s].\n", cone_session.session_name,
                           cone_session.session_path);
                    showPopup("Cone_Session", "Cone Session Started", "Cone session has been started.", NotificationType::Success);
                }
            }

            // Plot GPS Data
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (ImPlot::BeginPlot("GpsPositions", size, ImPlotFlags_Equal | ImPlotFlags_NoTitle)) {
                const MapInfo& selectedMap = maps[selectedMapIndex];

                // Set axis limits once or when the map changes
                if (resetView) {
                    float minLon = selectedMap.boundBL.x;
                    float maxLon = selectedMap.boundTR.x;
                    float minLat = selectedMap.boundBL.y;
                    float maxLat = selectedMap.boundTR.y;

                    for (const auto& cone : cones) {
                        if (cone.lon < minLon) minLon = cone.lon;
                        if (cone.lon > maxLon) maxLon = cone.lon;
                        if (cone.lat < minLat) minLat = cone.lat;
                        if (cone.lat > maxLat) maxLat = cone.lat;
                    }

                    // Set axis limits with margin
                    float margin_x = 0.0005f; 
                    float margin_y = 0.0005f;

                    ImPlot::SetupAxesLimits(
                        minLon - margin_x, maxLon + margin_x,
                        minLat - margin_y, maxLat + margin_y,
                        ImGuiCond_Always
                    );

                    ImPlot::SetupAxesLimits(
                        selectedMap.boundBL.x - margin_x, selectedMap.boundTR.x + margin_x,
                        selectedMap.boundBL.y - margin_y, selectedMap.boundTR.y + margin_y,
                        ImGuiCond_Always
                    );

                    // Disable resetView flag
                    resetView = false;
                }

                // Customize plot style
                ImPlot::GetStyle().LineWeight = 2.0f;
                ImPlot::GetStyle().MarkerSize = 6.0f;

                // Plot the map image
                if (selectedMap.texture != 0) {
                    ImPlot::PlotImage(selectedMap.name.c_str(), 
                                      selectedMap.texture, 
                                      selectedMap.boundBL, selectedMap.boundTR,
                                      ImVec2(0, 0), ImVec2(1, 1), 
                                      ImVec4(1, 1, 1, mapOpacity));
                }

                // Plot the trajectory
                if (!trajectory.empty()) {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    ImPlot::PlotScatter("Trajectory", &trajectory[0].x, &trajectory[0].y,
                                        static_cast<int>(trajectory.size()), 0, 0, sizeof(ImVec2));
                }

                // Plot cones and handle right-click detection
                for (size_t i = 0; i < cones.size(); ++i) {
                    ImVec4 color;
                    switch (cones[i].id) {
                    case CONE_ID_YELLOW:
                        color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                        break;
                    case CONE_ID_ORANGE:
                        color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
                        break;
                    case CONE_ID_BLUE:
                        color = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
                        break;
                    default:
                        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        break;
                    }
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Up, 8, color, 0.0);
                    ImPlot::PlotScatter(("Cone" + std::to_string(i)).c_str(), &cones[i].lon, &cones[i].lat, 1);
                }

                // Detect right-click to select a cone
                if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
                    float hitRadius = 0.001f;  // Adjust based on plot scale

                    // Find the closest cone to the mouse position
                    int closestConeIndex = findClosestCone(mousePos, cones, hitRadius);

                    if (closestConeIndex != -1) {
                        selectedConeIndex = closestConeIndex;  // Select the closest cone
                        showConeContextMenu = true;  // Show context menu for the selected cone
                    }
                }

                // Plot the current GPS position if valid
                if (currentPosition.x != 0.0f || currentPosition.y != 0.0f) {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 10, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.0);
                    ImPlot::PlotScatter("Current", &currentPosition.x, &currentPosition.y, 1);
                }

                // Move cone with a left-click on the map
                if (conePlacementMode && ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // Get mouse position in plot coordinates
                    ImPlotPoint plotMousePos = ImPlot::GetPlotMousePos();
                    // Update the position of the selected cone
                    if (selectedConeIndex >= 0 && selectedConeIndex < static_cast<int>(cones.size())) {
                        cones[selectedConeIndex].lon = plotMousePos.x;
                        cones[selectedConeIndex].lat = plotMousePos.y;
                        showPopup("Move_Cone_Completed", "Move Completed", "Cone moved successfully.", NotificationType::Success);
                        printf("Cone %d moved to: Lon=%f, Lat=%f\n", selectedConeIndex, plotMousePos.x, plotMousePos.y);
                    }
                    // Disable cone placement mode
                    conePlacementMode = false;
                }

                ImPlot::EndPlot();
            }

            // Handle Context Menu Popup
            if (showConeContextMenu && selectedConeIndex >= 0 && selectedConeIndex < static_cast<int>(cones.size())) {
                ImGui::OpenPopup("Cone Menu");
                showConeContextMenu = false;
            }

            if (ImGui::BeginPopup("Cone Menu")) {
                // Show icon based on cone type
                ImTextureID coneIcon = 0;
                switch (cones[selectedConeIndex].id) {
                    case CONE_ID_ORANGE:
                        coneIcon = getIconTexture("Success");
                        break;
                    case CONE_ID_YELLOW:
                        coneIcon = getIconTexture("Info");
                        break;
                    case CONE_ID_BLUE:
                        coneIcon = getIconTexture("Error");
                        break;
                    default:
                        break;
                }

                if (coneIcon != 0) {
                    ImGui::Image(coneIcon, ImVec2(24, 24));
                    ImGui::SameLine();
                }

                ImGui::Text("Modify Cone");
                ImGui::Separator();

                // Dropdown to select new color with tooltip
                const char* color_options[] = { "Orange", "Yellow", "Blue" };
                static int selected_color = 0;

                // Initialize selected_color based on current cone ID
                switch (cones[selectedConeIndex].id) {
                case CONE_ID_ORANGE:
                    selected_color = 0;
                    break;
                case CONE_ID_YELLOW:
                    selected_color = 1;
                    break;
                case CONE_ID_BLUE:
                    selected_color = 2;
                    break;
                default:
                    selected_color = 0;
                }

                if (ImGui::Combo("Color", &selected_color, color_options, IM_ARRAYSIZE(color_options))) {
                    // Update cone color based on selection
                    switch (selected_color) {
                    case 0:
                        cones[selectedConeIndex].id = CONE_ID_ORANGE;
                        break;
                    case 1:
                        cones[selectedConeIndex].id = CONE_ID_YELLOW;
                        break;
                    case 2:
                        cones[selectedConeIndex].id = CONE_ID_BLUE;
                        break;
                    }
                    showPopup("Cone_Color_Changed", "Color Changed", "The cone's color has been updated.", NotificationType::Success);
                    printf("Cone %d color changed to %s\n", selectedConeIndex, color_options[selected_color]);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Select a new color for the cone.");

                // Button to delete the cone with tooltip
                if (ImGui::Button("Delete")) {
                    cones.erase(cones.begin() + selectedConeIndex);
                    showPopup("Cone_Deleted", "Cone Deleted", "The selected cone has been deleted.", NotificationType::Info);
                    printf("Cone %d deleted.\n", selectedConeIndex);
                    selectedConeIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Delete the selected cone.");

                ImGui::SameLine();

                // Button to start moving the cone with tooltip
                if (ImGui::Button("Move Cone")) {
                    conePlacementMode = true;  // Enable cone placement mode
                    ImGui::CloseCurrentPopup();
                    showPopup("Move_Cone", "Move Cone", "Click on the map to move the cone to the desired position.", NotificationType::Info);
                    printf("Cone placement mode activated.\n");
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Move the selected cone to a new position.");

                ImGui::EndPopup();
            }

        }
        ImGui::End(); // End of the fullscreen window "ACR"

        endFrame(window);
    }

    // Cleanup
    kill_thread.store(true);
    if (gpsThread.joinable()) {
        gpsThread.join();
    }

    // Cleanup map textures
    for (auto& map : maps) {
        if (map.texture != 0) {
            GLuint texID = static_cast<GLuint>(reinterpret_cast<intptr_t>(map.texture));
            glDeleteTextures(1, &texID);
            map.texture = 0;
        }
    }

    // Cleanup icon textures
    for (auto& icon : icons) {
        if (icon.texture != 0) {
            GLuint texID = static_cast<GLuint>(reinterpret_cast<intptr_t>(icon.texture));
            glDeleteTextures(1, &texID);
            icon.texture = 0;
        }
    }
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
