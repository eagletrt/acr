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
    if (NFD_Init() != NFD_OKAY) {
        printf("Failed to initialize Native File Dialog.\n");
        return -1;
    }

    // Set the initial directory to Desktop
    std::string desktopPath = Utils::getDesktopPath();
    std::string currentPath = desktopPath;

    const char *basepath = getenv("HOME");
    if (basepath == nullptr) {
        printf("Error: HOME environment variable not set.\n");
        return -1;
    }

    // Create instances of managers
    GUI gui;
    if (!gui.setup()) {
        printf("Failed to initialize GUI.\n");
        return -1;
    }

    IconManager iconManager;

    // Pass iconManager to NotificationManager
    NotificationManager notificationManager(&iconManager);

    // Load icons with NotificationManager
    iconManager.loadIcons(notificationManager);

    FontManager fontManager;
    ImGuiIO& io = ImGui::GetIO();
    fontManager.initializeFonts(io, "../assets/fonts/");

    MapManager mapManager(notificationManager);
    // Add maps with their respective boundaries
    mapManager.addMap("Povo", "../assets/Povo.jpg", ImVec2{11.1484815430000008, 46.0658863580000002}, ImVec2{11.1515535430000003, 46.0689583580000033});
    mapManager.addMap("Vadena", "../assets/Vadena.jpg", ImVec2{11.3097566090000008, 46.4300119620000018}, ImVec2{11.3169246090000009, 46.4382039620000029});
    mapManager.addMap("FSG", "../assets/FSG.jpg", ImVec2{8.558931763076039, 49.32344089057215}, ImVec2{8.595726757113576, 49.335430701657735});
    mapManager.addMap("Ala", "../assets/Ala.jpg", ImVec2{11.010747212958533, 45.784567764275764}, ImVec2{11.013506837511347, 45.78713363420464});
    mapManager.addMap("Varano", "../assets/Varano.jpg", ImVec2{10.013347232876077, 44.67756187921092}, ImVec2{10.031744729894845, 44.684102509035036});

    // Load textures for all maps
    if (!mapManager.loadMapTextures()) {
        printf("Failed to load map textures.\n");
        return -1;
    }

    // Pass notificationManager to ConesLoader
    ConesLoader conesLoader(notificationManager);

    GPSManager gpsManager(notificationManager);
    // Initialize GPS with default port or file if provided
    if (argc > 1) {
        if (gpsManager.initialize(argv[1]) == -1) {
            printf("Error: Failed to initialize GPS with argument %s.\n", argv[1]);
            notificationManager.showPopup("GPS_Error", "Error", "Failed to initialize GPS with provided argument.", NotificationType::Error);
            // Decide whether to terminate or continue
        }
    } else {
        if (gpsManager.initialize("/dev/ttyACM0") == -1) { // Default serial port
            printf("Error: Failed to initialize GPS on default port /dev/ttyACM0.\n");
            notificationManager.showPopup("GPS_Error", "Error", "Failed to initialize GPS on default port.", NotificationType::Error);
            // Decide whether to terminate or continue
        }
    }
    gpsManager.start();

    FileBrowser fileBrowser(notificationManager);

    float mapOpacity = 0.5f;
    float lastTime = glfwGetTime();
    bool resetView = false;

    // Variables for the GPS dialog
    static bool showGPSDialog = false;
    static char serialPortInput[256] = "/dev/ttyACM0";

    // Main loop
    while (!gui.shouldClose()) {
        // Calculate delta time
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        gui.startFrame();

        // Process active notifications
        notificationManager.processNotifications(deltaTime);

        // Get the main viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportPos = viewport->Pos;
        ImVec2 viewportSize = viewport->Size;

        // Set the next window to cover the entire viewport
        ImGui::SetNextWindowPos(viewportPos);
        ImGui::SetNextWindowSize(viewportSize);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;

        // Begin the fullscreen window "ACR"
        if (ImGui::Begin("ACR", nullptr, window_flags)) {

            // Button to open the GPS dialog
            if (ImGui::Button("Open GPS")) {
                showGPSDialog = true;
                // Pre-fill serialPortInput with the default value
                strcpy(serialPortInput, "/dev/ttyACM0");
                ImGui::OpenPopup("Open GPS");
            }

            // Modal dialog to open GPS
            if (showGPSDialog) {
                if (ImGui::BeginPopupModal("Open GPS", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::InputText("Serial Port", serialPortInput, sizeof(serialPortInput));
                    if (ImGui::Button("OK")) {
                        // Attempt to initialize GPS with the entered serial port
                        // Stop the existing GPS session if active
                        gpsManager.stop();

                        // Close the previous GPS interface
                        gps_interface_close(&gpsManager.getGPS());

                        // Initialize GPS with the entered serial port
                        if (gpsManager.initialize(serialPortInput) == -1) {
                            printf("Error: GPS not found or failed to initialize on port %s.\n", serialPortInput);
                            notificationManager.showPopup("GPS_Error", "Error", "GPS not found or failed to initialize.", NotificationType::Error);
                        } else {
                            // Start the GPS reading thread
                            gpsManager.start();
                            printf("Started reading GPS data from serial port %s.\n", serialPortInput);
                            notificationManager.showPopup("GPS", "GPS Connected", "Reading GPS data from serial port.", NotificationType::Success);
                        }

                        showGPSDialog = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        showGPSDialog = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
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

                // Open the dialog window to select the log file
                nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

                if (result == NFD_OKAY && outPath != nullptr) {
                    printf("Selected file: %s\n", outPath);

                    // Stop the existing GPS thread if active
                    gpsManager.stop();

                    // Close the previous GPS interface
                    gps_interface_close(&gpsManager.getGPS());

                    // Open the selected log file
                    if (gps_interface_open_file(&gpsManager.getGPS(), outPath) == -1) {
                        printf("Error opening selected file: %s\n", outPath);
                        notificationManager.showPopup("FileBrowser", "Error", "Failed to open selected file.", NotificationType::Error);
                    } else {
                        // Reset session data and clear previous data
                        gpsManager.resetSessionData();

                        // Restart the GPS thread
                        gpsManager.start();

                        // Show success notification
                        notificationManager.showPopup("Success", "Success", "Successfully loaded log file.", NotificationType::Success);
                    }

                    // Free the path returned by NFD
                    NFD_FreePathU8(outPath);
                }
                else if (result == NFD_CANCEL) {
                    printf("User canceled file selection.\n");
                }
                else {
                    printf("Error selecting file: %s\n", NFD_GetError());
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

                // Open the dialog window to select the CSV file
                nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

                if (result == NFD_OKAY && outPath != nullptr) {
                    printf("Selected CSV file: %s\n", outPath);

                    // Load cones from the selected CSV file
                    if (conesLoader.loadFromCSV(outPath)) {
                        notificationManager.showPopup("Cones_Loaded", "Success", "Successfully loaded cones from CSV.", NotificationType::Success);
                        printf("Cones loaded from CSV: %s\n", outPath);
                        resetView = true; // Set resetView to update map limits
                    } else {
                        notificationManager.showPopup("Cones_Load_Error", "Error", "Failed to load cones from the selected CSV file.", NotificationType::Error);
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
                // "Help" Tab
                if (ImGui::BeginTabItem("Help")) {
                    ImGui::Text("Quit (Q)");
                    ImGui::Text("Toggle Trajectory Recording (T)");
                    ImGui::Text("Cones: ");
                    ImGui::Text("- Orange (O)");
                    ImGui::Text("- Yellow (Y)");
                    ImGui::Text("- Blue (B)");
                    ImGui::EndTabItem();
                }

                // "Settings" Tab
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
                    const auto& maps = mapManager.getMaps();
                    for (int i = 0; i < static_cast<int>(maps.size()); i++) {
                        if (ImGui::RadioButton(maps[i].name.c_str(), &mapManager.selectedMapIndex_, i)) {
                            // Show a confirmation notification when a map is selected
                            notificationManager.showPopup("Map", "Map Selected", "You have selected the map: " + maps[i].name, NotificationType::Success);
                            printf("Selected map: %s\n", maps[i].name.c_str());

                            // Reset the view when a new map is selected
                            resetView = true;
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Select the map to display.");
                    }

                    // Combo box for font selection with tooltip
                    if (ImGui::BeginCombo("Select Font", fontManager.getAvailableFonts()[fontManager.getSelectedFontIndex()].name.c_str())) {
                        const auto& availableFonts = fontManager.getAvailableFonts();
                        for (size_t n = 0; n < availableFonts.size(); n++) {
                            bool is_selected = (fontManager.getSelectedFontIndex() == static_cast<int>(n));
                            if (ImGui::Selectable(availableFonts[n].name.c_str(), is_selected))
                                fontManager.setSelectedFontIndex(static_cast<int>(n));
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Choose a font for the application.");

                    // Set the selected font
                    if (fontManager.getSelectedFontIndex() >= 0 && fontManager.getSelectedFontIndex() < static_cast<int>(fontManager.getAvailableFonts().size())) {
                        io.FontDefault = fontManager.getSelectedFont();
                    }

                    // Slider for font scale with tooltip
                    ImGui::BeginGroup();
                    ImGui::Text("Font Scale");
                    ImGui::SameLine();
                    ImGui::SliderFloat("##FontScale", &fontManager.fontScale_, 0.5f, 2.0f);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Adjust the global font scale.");
                    ImGui::EndGroup();

                    ImGui::Text("Font Scale: %.2f", fontManager.getFontScale());
                    io.FontGlobalScale = fontManager.getFontScale();

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::Separator();

            // Display HDOP with tooltip
            ImGui::Text("HDOP: %.2f [m]", gpsManager.getGPSData().hpposllh.hAcc);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Horizontal Dilution of Precision (HDOP) indicates the GPS accuracy.");

            {
                // Lock the mutex before accessing shared GPS data
                std::lock_guard<std::mutex> lock(gpsManager.getRenderLock());

                // Handle key presses with tooltip
                if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                    if (gpsManager.getSession().active) {
                        csv_session_stop(&gpsManager.getSession());
                        printf("Session '%s' ended.\n", gpsManager.getSession().session_name);
                        notificationManager.showPopup("Session", "Session Stopped", "Recording session ended.", NotificationType::Info);
                    }
                    else {
                        if (csv_session_setup(&gpsManager.getSession(), basepath) == -1) {
                            printf("Error: Session setup failed.\n");
                            notificationManager.showPopup("Session", "Error", "Session setup failed.", NotificationType::Error);
                        }
                        if (csv_session_start(&gpsManager.getSession()) == -1) {
                            printf("Error: Session start failed.\n");
                            notificationManager.showPopup("Session", "Error", "Session start failed.", NotificationType::Error);
                        }
                        printf("Session '%s' started [%s].\n", gpsManager.getSession().session_name,
                               gpsManager.getSession().session_path);
                        notificationManager.showPopup("Session", "Session Started", "Recording session started.", NotificationType::Info);
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_O)) {
                    gpsManager.setConeId(CONE_ID_ORANGE);
                    gpsManager.saveCone_.store(true);
                    notificationManager.showPopup("Cone_Orange", "Cone Placed", "An orange cone has been placed.", NotificationType::Success);
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
                    gpsManager.setConeId(CONE_ID_YELLOW);
                    gpsManager.saveCone_.store(true);
                    notificationManager.showPopup("Cone_Yellow", "Cone Placed", "A yellow cone has been placed.", NotificationType::Success);
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_B)) {
                    gpsManager.setConeId(CONE_ID_BLUE);
                    gpsManager.saveCone_.store(true);
                    notificationManager.showPopup("Cone_Blue", "Cone Placed", "A blue cone has been placed.", NotificationType::Success);
                }


                if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
                    glfwSetWindowShouldClose(gui.getWindow(), true);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_C)) {
                    gpsManager.getTrajectory().clear();
                    gpsManager.getCones().clear();
                    printf("Trajectory and cones cleared.\n");
                    notificationManager.showPopup("Cleared", "Cleared", "Trajectory and cones have been cleared.", NotificationType::Info);
                }
                if (gpsManager.saveCone_.load() && gpsManager.getConeSession().active == 0) {
                    if (cone_session_setup(&gpsManager.getConeSession(), basepath) == -1) {
                        printf("Error: Cone session setup failed.\n");
                        notificationManager.showPopup("Cone_Session", "Error", "Cone session setup failed.", NotificationType::Error);
                    }
                    if (cone_session_start(&gpsManager.getConeSession()) == -1) {
                        printf("Error: Cone session start failed.\n");
                        notificationManager.showPopup("Cone_Session", "Error", "Cone session start failed.", NotificationType::Error);
                    }
                    printf("Cone session '%s' started [%s].\n", gpsManager.getConeSession().session_name,
                           gpsManager.getConeSession().session_path);
                    notificationManager.showPopup("Cone_Session", "Cone Session Started", "Cone session has been started.", NotificationType::Success);
                }
            }

            // Plot GPS data
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (ImPlot::BeginPlot("GpsPositions", size, ImPlotFlags_Equal | ImPlotFlags_NoTitle)) {
                const MapInfo& selectedMap = mapManager.getMaps()[mapManager.getSelectedMapIndex()];

                // Set axis limits once or when the map changes
                if (resetView) {
                    float minLon = selectedMap.boundBL.x;
                    float maxLon = selectedMap.boundTR.x;
                    float minLat = selectedMap.boundBL.y;
                    float maxLat = selectedMap.boundTR.y;

                    auto& cones = gpsManager.getCones();
                    for (const auto& cone : cones) {
                        if (cone.lon < minLon) minLon = cone.lon;
                        if (cone.lon > maxLon) maxLon = cone.lon;
                        if (cone.lat < minLat) minLat = cone.lat;
                        if (cone.lat > maxLat) maxLat = cone.lat;
                    }

                    // Set axis limits with a margin
                    float margin_x = 0.0005f; 
                    float margin_y = 0.0005f;

                    ImPlot::SetupAxesLimits(
                        minLon - margin_x, maxLon + margin_x,
                        minLat - margin_y, maxLat + margin_y,
                        ImGuiCond_Always
                    );

                    // Disable the resetView flag
                    resetView = false;
                }

                // Customize plot style
                ImPlot::GetStyle().LineWeight = 2.0f;
                ImPlot::GetStyle().MarkerSize = 6.0f;

                // Plot the map image
                if (selectedMap.texture != 0) {
                    ImPlot::PlotImage(selectedMap.name.c_str(), 
                                      selectedMap.texture, 
                                      ImPlotPoint(selectedMap.boundBL.x, selectedMap.boundBL.y), 
                                      ImPlotPoint(selectedMap.boundTR.x, selectedMap.boundTR.y),
                                      ImVec2(0, 0), ImVec2(1, 1), 
                                      ImVec4(1, 1, 1, mapOpacity));
                }

                // Plot the trajectory
                const auto& trajectory = gpsManager.getTrajectory();
                if (!trajectory.empty()) {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    ImPlot::PlotScatter("Trajectory", &trajectory[0].x, &trajectory[0].y,
                                        static_cast<int>(trajectory.size()), 0, 0, sizeof(ImVec2));
                }

                // Plot cones and handle right-click detection
                auto& cones = gpsManager.getCones(); // Modification: use a non-const reference
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
                    int closestConeIndex = mapManager.findClosestCone(mousePos, cones, hitRadius);

                    if (closestConeIndex != -1) {
                        mapManager.selectedConeIndex_ = closestConeIndex;  // Select the closest cone
                        mapManager.showConeContextMenu_ = true;  // Show context menu for the selected cone
                    }
                }

                // Plot the current GPS position if valid
                ImVec2 currentPos = gpsManager.getCurrentPosition();
                if (currentPos.x != 0.0f || currentPos.y != 0.0f) {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 10, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.0);
                    ImPlot::PlotScatter("Current", &currentPos.x, &currentPos.y, 1);
                }

                // Move the cone with a left-click on the map
                if (gpsManager.getConePlacementMode() && ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // Get the mouse position in the plot
                    ImPlotPoint plotMousePos = ImPlot::GetPlotMousePos();
                    // Update the position of the selected cone
                    if (mapManager.selectedConeIndex_ >= 0 && mapManager.selectedConeIndex_ < static_cast<int>(cones.size())) {
                        cones[mapManager.selectedConeIndex_].lon = plotMousePos.x;
                        cones[mapManager.selectedConeIndex_].lat = plotMousePos.y;
                        notificationManager.showPopup("Move_Cone_Completed", "Move Completed", "Cone moved successfully.", NotificationType::Success);
                        printf("Cone %d moved to: Lon=%f, Lat=%f\n", mapManager.selectedConeIndex_, plotMousePos.x, plotMousePos.y);
                    }
                    // Disable cone placement mode
                    gpsManager.setConePlacementMode(false);
                }

                ImPlot::EndPlot();
            }

            // Handle the context menu for cones
            if (mapManager.showConeContextMenu_ && mapManager.selectedConeIndex_ >= 0 && mapManager.selectedConeIndex_ < static_cast<int>(gpsManager.getCones().size())) {
                ImGui::OpenPopup("Cone Menu");
                mapManager.showConeContextMenu_ = false;
            }

            if (ImGui::BeginPopup("Cone Menu")) {
                // Show the icon based on the type of cone
                ImTextureID coneIcon = 0;
                auto& cones = gpsManager.getCones();
                switch (cones[mapManager.selectedConeIndex_].id) {
                    case CONE_ID_ORANGE:
                        coneIcon = iconManager.getIconTexture("Success");
                        break;
                    case CONE_ID_YELLOW:
                        coneIcon = iconManager.getIconTexture("Info");
                        break;
                    case CONE_ID_BLUE:
                        coneIcon = iconManager.getIconTexture("Error");
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

                // Dropdown to select the new color with tooltip
                const char* color_options[] = { "Orange", "Yellow", "Blue" };
                static int selected_color = 0;

                // Initialize selected_color based on the current cone ID
                switch (cones[mapManager.selectedConeIndex_].id) {
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
                    // Update the cone's color based on selection
                    switch (selected_color) {
                    case 0:
                        cones[mapManager.selectedConeIndex_].id = CONE_ID_ORANGE;
                        break;
                    case 1:
                        cones[mapManager.selectedConeIndex_].id = CONE_ID_YELLOW;
                        break;
                    case 2:
                        cones[mapManager.selectedConeIndex_].id = CONE_ID_BLUE;
                        break;
                    }
                    notificationManager.showPopup("Cone_Color_Changed", "Color Changed", "The cone's color has been updated.", NotificationType::Success);
                    printf("Cone %d color changed to %s\n", mapManager.selectedConeIndex_, color_options[selected_color]);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Select a new color for the cone.");

                // Button to delete the cone with tooltip
                if (ImGui::Button("Delete")) {
                    gpsManager.deleteCone(mapManager.selectedConeIndex_);
                    notificationManager.showPopup("Cone_Deleted", "Cone Deleted", "The selected cone has been deleted.", NotificationType::Info);
                    printf("Cone %d deleted.\n", mapManager.selectedConeIndex_);
                    mapManager.selectedConeIndex_ = -1;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Delete the selected cone.");

                ImGui::SameLine();

                // Button to start moving the cone with tooltip
                if (ImGui::Button("Move Cone")) {
                    gpsManager.setConePlacementMode(true);  // Enable cone placement mode
                    ImGui::CloseCurrentPopup();
                    notificationManager.showPopup("Move_Cone", "Move Cone", "Click on the map to move the cone to the desired position.", NotificationType::Info);
                    printf("Cone placement mode activated.\n");
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Move the selected cone to a new position.");

                ImGui::EndPopup();
            }

        }
        ImGui::End(); // End of the fullscreen window "ACR"

        gui.endFrame();
    }

    // Cleanup
    gpsManager.stop();
    gui.cleanup();
    NFD_Quit();

    return 0;
}
