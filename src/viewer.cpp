#define STB_IMAGE_IMPLEMENTATION
#include "gui.hpp"
#include "config.hpp"
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
#include <future>
#include <sstream>
#include <cfloat>
#include "map.hpp"

extern "C" {
    #include "acr.h"
    #include "defines.h"
    #include "main.h"
    #include "utils.h"
}

#include "gps.hpp"
#include "notifications.hpp"

#include "font_manager.hpp"
#include "icon_manager.hpp"
#include "utils.hpp"
#include "cones_loader.hpp"

struct BoolWrapper {
    bool value;

    BoolWrapper(bool val = true) : value(val) {}
};

extern void setEnhancedTheme();

bool LoadConfig(AppTheme& theme, int& lastFontIndex, std::string& currentPath) {
    std::ifstream configFile("config.ini");
    if (!configFile.is_open()) {
        theme = AppTheme::Dark;
        lastFontIndex = 0;
        currentPath = Utils::getDesktopPath(); 
        return false;
    }
    std::string line;
    bool themeSet = false;
    bool fontSet = false;
    bool pathSet = false;
    while (std::getline(configFile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "theme") {
                if (value == "Dark") {
                    theme = AppTheme::Dark;
                    themeSet = true;
                } else if (value == "Blue") {
                    theme = AppTheme::Blue;
                    themeSet = true;
                } else if (value == "Light") {
                    theme = AppTheme::Light;
                    themeSet = true;
                }
            } else if (key == "last_font") {
                try {
                    lastFontIndex = std::stoi(value);
                    fontSet = true;
                } catch (...) {
                    lastFontIndex = 0;
                }
            } else if (key == "current_path") {
                currentPath = value;
                pathSet = true;
            }
        }
    }
    configFile.close();
    if (!themeSet) {
        theme = AppTheme::Dark;
    }
    if (!fontSet) {
        lastFontIndex = 0;
    }
    if (!pathSet) {
        currentPath = Utils::getDesktopPath();
    }
    return themeSet && fontSet && pathSet;
}

bool SaveConfig(const AppTheme& theme, int lastFontIndex, const std::string& currentPath) {
    std::ofstream configFile("config.ini", std::ios::out | std::ios::trunc);
    if (!configFile.is_open()) {
        printf("Failed to open config file for writing.\n");
        return false;
    }
    std::string themeStr;
    switch (theme) {
        case AppTheme::Dark:
            themeStr = "Dark";
            break;
        case AppTheme::Blue:
            themeStr = "Blue";
            break;
        case AppTheme::Light:
            themeStr = "Light";
            break;
    }
    configFile << "theme=" << themeStr << "\n";
    configFile << "last_font=" << lastFontIndex << "\n";
    configFile << "current_path=" << currentPath << "\n";
    configFile.close();
    return true;
}

float CalculateDistance(float x1, float y1, float x2, float y2) {
    return sqrtf((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
}

int main(int argc, char **argv) {
    
    if (NFD_Init() != NFD_OKAY) {
        printf("Failed to initialize Native File Dialog.\n");
        return -1;
    }

    std::future<std::string> fileDialogFuture;
    std::atomic<bool> fileDialogActive(false);
    std::string selectedFilePath;

    
    std::future<std::string> conesDialogFuture;
    std::atomic<bool> conesDialogActive(false);
    std::string selectedConesPath;
    std::string currentPath;


    
    std::string desktopPath = Utils::getDesktopPath();

    const char* home_env = getenv("HOME");
    if (home_env == nullptr) {
        printf("Error: HOME environment variable not set.\n");
        return -1;
    }
    std::string basepath(home_env);
    std::string logs_v2_basepath = basepath + "/logs-v2";

    if (!std::filesystem::exists(logs_v2_basepath)) {
        if (!std::filesystem::create_directories(logs_v2_basepath)) {
            printf("Error: Could not create directory %s\n", logs_v2_basepath.c_str());
            return -1;
        }
    }

    
    GUI gui;
    if (!gui.setup()) {
        printf("Failed to initialize GUI.\n");
        return -1;
    }

    
    IconManager iconManager;

    
    NotificationManager notificationManager(&iconManager);

    
    iconManager.loadIcons(notificationManager);

    
    FontManager fontManager;
    ImGuiIO& io = ImGui::GetIO();

    
    AppTheme currentTheme = AppTheme::Dark;
    int lastFontIndex = 0;
    LoadConfig(currentTheme, lastFontIndex, currentPath);

    
    fontManager.initializeFonts(io, FONTS_DIR, lastFontIndex);

    
    ApplyTheme(currentTheme); 
    SetImPlotStyle(currentTheme);

    
    MapManager mapManager(notificationManager);
    
    
    mapManager.addMap("Povo", MAPS_DIR "Povo.jpg", ImPlotPoint{11.148481543, 46.065886358}, ImPlotPoint{11.151553543, 46.068958358});
    mapManager.addMap("Vadena", MAPS_DIR "Vadena.jpg", ImPlotPoint{11.309756609, 46.430011962}, ImPlotPoint{11.316924609, 46.438203962});
    mapManager.addMap("FSG", MAPS_DIR "FSG.jpg", ImPlotPoint{8.558931763, 49.323440890}, ImPlotPoint{8.595726757, 49.335430701});
    mapManager.addMap("Ala", MAPS_DIR "Ala.jpg", ImPlotPoint{11.010747213, 45.784567764}, ImPlotPoint{11.013506837, 45.787133634});
    mapManager.addMap("Varano", MAPS_DIR "Varano.jpg", ImPlotPoint{10.013347233, 44.677561879}, ImPlotPoint{10.031744730, 44.684102509});

    
    if (!mapManager.loadMapTextures()) {
        printf("Failed to load initial map textures.\n");
        return -1;
    }

    
    ConesLoader conesLoader(notificationManager);

    
    GPSManager gpsManager(notificationManager);
    
    if (argc > 1) {
        if (gpsManager.initialize(argv[1]) == -1) {
            printf("Error: Failed to initialize GPS with argument %s.\n", argv[1]);
            notificationManager.showPopup("GPS_Error", "Error", "Failed to initialize GPS with provided argument.", NotificationType::Error);
        }
    } else {
        if (gpsManager.initialize(DEFAULT_GPS_PORT) == -1) { 
            printf("Error: Failed to initialize GPS on default port %s.\n", DEFAULT_GPS_PORT);
            notificationManager.showPopup("GPS_Error", "Error", "Failed to initialize GPS on default port.", NotificationType::Error);
        }
    }
    gpsManager.start();


    float mapOpacity = 0.5f;
    float lastTime = glfwGetTime();
    bool resetView = false;

    
    std::vector<float> hdopValues;
    std::vector<float> pdopValues; 
    std::vector<float> timeValues;
    float plotTime = 0.0f;
    float windowSize = 60.0f; 

    
    static bool showGPSDialog = false;
    static char serialPortInput[256] = DEFAULT_GPS_PORT;

    
    bool showTrajectory = true;
    bool showCurrentPosition = true;
    std::vector<BoolWrapper> coneVisibility;

    
    ImPlotPoint lastPlotPos(10, 10); 
    ImPlotPoint lastPlotSize(0, 0);

    
    bool isDragging = false;
    int draggedConeIndex = -1;

    
    bool dragStarted = false;
    bool dragEnded = false;

    
    while (!gui.shouldClose()) {
        
        float currentTimeSec = glfwGetTime();
        float deltaTime = currentTimeSec - lastTime;
        lastTime = currentTimeSec;

        mapManager.processPendingTextures();

        
        gui.startFrame();

        
        notificationManager.processNotifications(deltaTime);

        
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;

        if (ImGui::Begin("ACR", nullptr, window_flags)) {
            
            ImGui::BeginGroup(); 

            
            if (ImGui::Button("Open GPS")) {
                showGPSDialog = true;
                strcpy(serialPortInput, DEFAULT_GPS_PORT);
                ImGui::OpenPopup("Open GPS");
            }
            ImGui::SameLine();

            
            if (ImGui::Button("Load Log File")) {
                if (!fileDialogActive.load()) {
                    auto promisePtr = std::make_shared<std::promise<std::string>>();
                    fileDialogFuture = promisePtr->get_future();
                    fileDialogActive.store(true);

                    std::thread([promisePtr, currentPath]() mutable {
                        nfdu8filteritem_t filterList[] = {
                            { "Log files", "log,txt" }
                        };
                        size_t filterCount = sizeof(filterList) / sizeof(filterList[0]);

                        nfdopendialogu8args_t args = {0};
                        args.filterList = filterList;
                        args.filterCount = filterCount;
                        args.defaultPath = currentPath.c_str();

                        nfdu8char_t* outPath = nullptr;

                        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

                        if (result == NFD_OKAY && outPath != nullptr) {
                            std::string selectedFile(outPath);
                            NFD_FreePathU8(outPath);
                            promisePtr->set_value(selectedFile);
                        } else {
                            promisePtr->set_value("");
                        }
                    }).detach();
                }
            }
            ImGui::SameLine();

            
            if (ImGui::Button("Load Cones CSV")) {
                if (!conesDialogActive.load()) {
                    auto promisePtr = std::make_shared<std::promise<std::string>>();
                    conesDialogFuture = promisePtr->get_future();
                    conesDialogActive.store(true);

                    std::thread([promisePtr, currentPath]() mutable {
                        nfdu8filteritem_t filterList[] = {
                            { "CSV files", "csv" }
                        };
                        size_t filterCount = sizeof(filterList) / sizeof(filterList[0]);

                        nfdopendialogu8args_t args = {0};
                        args.filterList = filterList;
                        args.filterCount = filterCount;
                        args.defaultPath = currentPath.c_str();

                        nfdu8char_t* outPath = nullptr;

                        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

                        if (result == NFD_OKAY && outPath != nullptr) {
                            std::string selectedFile(outPath);
                            NFD_FreePathU8(outPath);
                            promisePtr->set_value(selectedFile);
                        } else {
                            promisePtr->set_value("");
                        }
                    }).detach();
                }
            }

            ImGui::EndGroup();

            
            ImGui::Spacing();

            
            if (showGPSDialog) {
                if (ImGui::BeginPopupModal("Open GPS", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::InputText("Serial Port", serialPortInput, sizeof(serialPortInput));
                    if (ImGui::Button("OK")) {
                        
                        gpsManager.stop();

                        
                        gps_interface_close(&gpsManager.getGPS());

                        
                        if (gpsManager.initialize(serialPortInput) == -1) {
                            printf("Error: GPS not found or failed to initialize on port %s.\n", serialPortInput);
                            notificationManager.showPopup("GPS_Error", "Error", "GPS not found or failed to initialize.", NotificationType::Error);
                        } else {
                            
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

            
            if (ImGui::BeginTabBar("MainTabBar")) {
                
                if (ImGui::BeginTabItem("Help")) {
                    if (ImGui::CollapsingHeader("Controls")) {
                        ImGui::Text("Quit (Q)");
                        ImGui::Text("Toggle Trajectory Recording (T)");
                        ImGui::Text("Cones:");
                        
                        
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 255)); 
                        ImGui::Text("- Orange (O)");
                        ImGui::PopStyleColor();

                        
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255)); 
                        ImGui::Text("- Yellow (Y)");
                        ImGui::PopStyleColor();

                        
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 255, 255)); 
                        ImGui::Text("- Blue (B)");
                        ImGui::PopStyleColor();
                    }
                    ImGui::EndTabItem();
                }

                
                if (ImGui::BeginTabItem("Settings")) {
                    if (ImGui::CollapsingHeader("Map Settings")) {
                        
                        ImGui::BeginGroup();
                        ImGui::Text("Map Opacity");
                        ImGui::SameLine();
                        ImGui::SliderFloat("##MapOpacity", &mapOpacity, 0.0f, 1.0f);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Adjust the opacity of the map overlay.");
                        ImGui::EndGroup();

                        
                        ImGui::Text("Select Map:");
                        const auto& maps = mapManager.getMaps();
                        for (size_t i = 0; i < maps.size(); i++) {
                            if (ImGui::RadioButton(maps[i].name.c_str(), &mapManager.selectedMapIndex_, i)) {
                                notificationManager.showPopup("MapManager", "Map Selected", "You have selected the map: " + maps[i].name, NotificationType::Success);
                                printf("Selected map: %s\n", maps[i].name.c_str());
                                resetView = true;
                            }
                            if(i != maps.size() - 1) {
                              ImGui::SameLine();
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Select the map to display.");
                            }
                        }
                    }

                    if (ImGui::CollapsingHeader("Font Settings")) {
                        
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

                        
                        if (fontManager.getSelectedFontIndex() >= 0 && fontManager.getSelectedFontIndex() < static_cast<int>(fontManager.getAvailableFonts().size())) {
                            io.FontDefault = fontManager.getSelectedFont();
                            
                            SaveConfig(currentTheme, fontManager.getSelectedFontIndex(), currentPath);
                        }

                        
                        ImGui::BeginGroup();
                        ImGui::Text("Font Scale");
                        ImGui::SameLine();
                        ImGui::SliderFloat("##FontScale", &fontManager.fontScale_, 0.5f, 2.0f);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Adjust the global font scale.");
                        ImGui::EndGroup();

                        ImGui::Text("Font Scale: %.2f", fontManager.getFontScale());
                        io.FontGlobalScale = fontManager.getFontScale();
                    }
                    
                    
                    if (ImGui::CollapsingHeader("Theme Settings")) {
                        const char* themes[] = { "Dark", "Blue", "Light" };
                        static int selectedThemeIndex = static_cast<int>(currentTheme);
                        if (ImGui::Combo("Select Theme", &selectedThemeIndex, themes, IM_ARRAYSIZE(themes))) {
                            currentTheme = static_cast<AppTheme>(selectedThemeIndex);
                            ApplyTheme(currentTheme);
                            SetImPlotStyle(currentTheme);
                            notificationManager.showPopup("Theme", "Theme Changed", "The application theme has been updated.", NotificationType::Info);
                            
                            SaveConfig(currentTheme, fontManager.getSelectedFontIndex(), currentPath);
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Choose a theme for the application.");
                    }

                    
                    if (ImGui::CollapsingHeader("Reset Settings")) {
                        if (ImGui::Button("Reset Trajectory and Cones")) {
                            gpsManager.resetSessionData();
                            gpsManager.clearCones();
                            notificationManager.showPopup("Reset", "Reset", "Trajectory and cones have been reset.", NotificationType::Info);
                            printf("Trajectory and cones have been reset.\n");
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Click to reset the trajectory and all loaded cones.");
                    }

                    ImGui::EndTabItem();
                }

                
                if (ImGui::BeginTabItem("Statistics")) {
                    if (ImGui::CollapsingHeader("GPS Accuracy (DOP)")) {
                        
                        if (ImPlot::BeginPlot("DOP Over Time", ImVec2(-1, 300))) {
                            
                            ImPlot::SetupAxes("Time (s)", "DOP", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                            ImPlotStyle& plotStyle = ImPlot::GetStyle();

                            
                            plotStyle.Colors[ImPlotCol_AxisGrid] = ImVec4(0.5f, 0.5f, 0.5f, 0.3f); 

                            
                            ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 1.5f); 
                            ImPlot::PlotLine("HDOP", timeValues.data(), hdopValues.data(), hdopValues.size());

                            
                            ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 1.5f); 
                            ImPlot::PlotLine("PDOP", timeValues.data(), pdopValues.data(), pdopValues.size());

                            ImPlot::EndPlot();
                        }
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::Separator();

            
            float currentHDOP = gpsManager.getGPSData().dop.hDOP;
            float currentPDOP = gpsManager.getGPSData().dop.pDOP;

            ImGui::Text("HDOP: %.2f", currentHDOP);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Horizontal Dilution of Precision (HDOP) indicates the horizontal accuracy of the GPS.");

            ImGui::Text("PDOP: %.2f", currentPDOP);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Position Dilution of Precision (PDOP) indicates the overall accuracy of the GPS.");

            
            hdopValues.push_back(currentHDOP);
            pdopValues.push_back(currentPDOP);
            timeValues.push_back(plotTime);
            plotTime += deltaTime;

            
            while (!timeValues.empty() && (plotTime - timeValues.front()) > windowSize) {
                hdopValues.erase(hdopValues.begin());
                pdopValues.erase(pdopValues.begin());
                timeValues.erase(timeValues.begin());
            }

            {
                
                std::lock_guard<std::mutex> lock(gpsManager.getRenderLock());

                
                if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                    if (gpsManager.getSession().active) {
                        csv_session_stop(&gpsManager.getSession());
                        printf("Session '%s' ended.\n", gpsManager.getSession().session_name);
                        notificationManager.showPopup("Session", "Session Stopped", "Recording session ended.", NotificationType::Info);
                    }
                    else {
                        if (csv_session_setup(&gpsManager.getSession(), logs_v2_basepath.c_str()) == -1) {
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
                    gpsManager.resetSessionData();
                    gpsManager.clearCones();
                    notificationManager.showPopup("Cleared", "Cleared", "Trajectory and cones have been cleared.", NotificationType::Info);
                    printf("Trajectory and cones have been reset.\n");
                }

                
                if (gpsManager.saveCone_.load() && gpsManager.getConeSession().active == 0) {
                    if (cone_session_setup(&gpsManager.getConeSession(), logs_v2_basepath.c_str()) == -1) {
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

            
            if (ImGui::Button("Show Legend")) {
                ImGui::OpenPopup("Legend Popup");
            }

            if (ImGui::BeginPopup("Legend Popup")) {
                ImGui::Text("Legend Controls");
                ImGui::Separator();

                ImGui::Checkbox("Trajectory", &showTrajectory);
                ImGui::Checkbox("Current Position", &showCurrentPosition);

                if (ImGui::TreeNode("Cones")) {
                    auto& conesList = gpsManager.getCones();
                    if (coneVisibility.size() != conesList.size()) {
                        coneVisibility.resize(conesList.size(), BoolWrapper{true});
                    }

                    ImGui::BeginChild("ConesList", ImVec2(260, 80), true, ImGuiWindowFlags_NoScrollbar);

                    for (size_t i = 0; i < conesList.size(); ++i) {
                        char label[64];
                        snprintf(label, sizeof(label), "Cone %zu", i);
                        ImGui::Checkbox(label, &coneVisibility[i].value);
                    }

                    ImGui::EndChild();
                    ImGui::TreePop();
                }

                ImGui::EndPopup();
            }

            
            ImVec2 size = ImGui::GetContentRegionAvail();

            bool plotRendered = false; 

            
            if (ImPlot::BeginPlot("GpsPositions", size, ImPlotFlags_Equal | ImPlotFlags_NoTitle | ImPlotFlags_NoLegend )) {
                plotRendered = true;

                const MapInfo& selectedMap = mapManager.getMaps()[mapManager.getSelectedMapIndex()];

                
                float minLon, maxLon, minLat, maxLat;
                {
                    std::lock_guard<std::mutex> lock(mapManager.mapMutex_);
                    minLon = selectedMap.boundBL.x;
                    maxLon = selectedMap.boundTR.x;
                    minLat = selectedMap.boundBL.y;
                    maxLat = selectedMap.boundTR.y;
                }

                
                auto& cones = gpsManager.getCones();
                for (const auto& cone : cones) {
                    if (cone.lon < minLon) minLon = cone.lon;
                    if (cone.lon > maxLon) maxLon = cone.lon;
                    if (cone.lat < minLat) minLat = cone.lat;
                    if (cone.lat > maxLat) maxLat = cone.lat;
                }

                
                float margin_x = 0.0005f; 
                float margin_y = 0.0005f;

                
                if (resetView) {
                    ImPlot::SetupAxisLimits(ImAxis_X1, minLon - margin_x, maxLon + margin_x, ImPlotCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, minLat - margin_y, maxLat + margin_y, ImPlotCond_Always);
                    resetView = false;
                }

                
                ImPlot::GetStyle().LineWeight = 2.0f;
                ImPlot::GetStyle().MarkerSize = 6.0f;

                
                ImPlot::SetupLegend(ImPlotLocation_NorthEast);

                
                {
                    std::lock_guard<std::mutex> lock(mapManager.mapMutex_);
                    if (selectedMap.texture != 0) {
                        ImPlot::PlotImage(selectedMap.name.c_str(), 
                                          selectedMap.texture, 
                                          ImPlotPoint(selectedMap.boundBL.x, selectedMap.boundBL.y), 
                                          ImPlotPoint(selectedMap.boundTR.x, selectedMap.boundTR.y),
                                          ImVec2(0, 0), ImVec2(1, 1), 
                                          ImVec4(1, 1, 1, mapOpacity));
                    }
                }

                
                const auto& trajectory = gpsManager.getTrajectory();
                if (showTrajectory && !trajectory.empty()) {
                    ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f); 
                    std::vector<float> trajX, trajY;
                    trajX.reserve(trajectory.size());
                    trajY.reserve(trajectory.size());
                    for (const auto& pos : trajectory) {
                        trajX.push_back(pos.x);
                        trajY.push_back(pos.y);
                    }
                    ImPlot::PlotLine("Trajectory", trajX.data(), trajY.data(),
                                    static_cast<int>(trajX.size()), 0, 0, sizeof(float));
                }

                
                if (coneVisibility.size() != cones.size()) {
                    coneVisibility.resize(cones.size(), BoolWrapper{true});
                }

                ImVec2 mouseScreen = ImGui::GetMousePos();
                const float hitRadiusPx = 10.0f; 
                int closestConeIndex = -1;
                float minDistPx = FLT_MAX;

                for (size_t i = 0; i < cones.size(); ++i) {
                    if (!coneVisibility[i].value) 
                        continue;

                    ImVec2 coneScreen = ImPlot::PlotToPixels(
                        ImPlotPoint(cones[i].lon, cones[i].lat)
                    );

                    float dx = mouseScreen.x - coneScreen.x;
                    float dy = mouseScreen.y - coneScreen.y;
                    float distPx = sqrtf(dx*dx + dy*dy);

                    if (distPx < hitRadiusPx && distPx < minDistPx) {
                        closestConeIndex = static_cast<int>(i);
                        minDistPx = distPx;
                    }
                }
                
                if (!isDragging && closestConeIndex != -1 
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Left) 
                    && ImPlot::IsPlotHovered()) {
                    isDragging = true;
                    draggedConeIndex = closestConeIndex;
                    notificationManager.showPopup(
                        "Drag_Cone_Start", 
                        "Drag Started", 
                        "Dragging cone started.", 
                        NotificationType::Info
                    );
                }
                

                if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    if (draggedConeIndex >= 0 && draggedConeIndex < static_cast<int>(cones.size())) {
                        ImPlotPoint newPos = ImPlot::GetPlotMousePos();
                        
                        {
                            
                            std::lock_guard<std::mutex> lock(gpsManager.getRenderLock());
                            gpsManager.getCones()[draggedConeIndex].lon = newPos.x;
                            gpsManager.getCones()[draggedConeIndex].lat = newPos.y;
                        }

                        printf("Dragging Cone %d to Lon=%f, Lat=%f\n", draggedConeIndex, newPos.x, newPos.y);
                    }
                }

                if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    if (draggedConeIndex >= 0 && draggedConeIndex < static_cast<int>(cones.size())) {
                        printf("Stopped dragging Cone %d\n", draggedConeIndex);
                        notificationManager.showPopup("Drag_Cone_End", "Drag Completed", "Cone moved successfully.", NotificationType::Success);
                    }
                    isDragging = false;
                    draggedConeIndex = -1;
                    dragEnded = true;
                }

                
                if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    if (closestConeIndex != -1 && coneVisibility[closestConeIndex].value) {
                        mapManager.selectedConeIndex_ = closestConeIndex;  
                        mapManager.showConeContextMenu_ = true;            
                    }
                }

                
                for (size_t i = 0; i < cones.size(); ++i) {
                    if (coneVisibility[i].value) {
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

                        
                        if (isDragging && static_cast<int>(i) == draggedConeIndex) {
                            color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); 
                        }

                        
                        std::string coneLabel = "Cone" + std::to_string(i);

                        
                        ImPlot::SetNextMarkerStyle(ImPlotMarker_Up, 8, color, 0.0f);

                        
                        ImPlot::PlotScatter(coneLabel.c_str(), &cones[i].lon, &cones[i].lat, 1);
                    }
                }

                
                ImPlotPoint currentPos = gpsManager.getCurrentPosition();
                if (showCurrentPosition && (currentPos.x != 0.0f || currentPos.y != 0.0f)) {
                    constexpr auto col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 8, ImVec4(0.0, 0.0, 0.0, 0.0), 3.0f, col);
                    ImPlot::PlotScatter("Current Position", &currentPos.x, &currentPos.y, 1);
                }

                ImPlot::EndPlot();
            }

            
            if (mapManager.showConeContextMenu_ && 
                mapManager.selectedConeIndex_ >= 0 && 
                mapManager.selectedConeIndex_ < static_cast<int>(gpsManager.getCones().size())) {
                ImGui::OpenPopup("Cone Menu");
                mapManager.showConeContextMenu_ = false;
            }

            if (ImGui::BeginPopup("Cone Menu")) {
                
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

                
                const char* color_options[] = { "Orange", "Yellow", "Blue" };
                static int selected_color = 0;

                
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

                
                if (ImGui::Button("Delete")) {
                    gpsManager.deleteCone(mapManager.selectedConeIndex_);
                    notificationManager.showPopup("Cone_Deleted", "Cone Deleted", "The selected cone has been deleted.", NotificationType::Info);
                    printf("Cone %d deleted.\n", mapManager.selectedConeIndex_);
                    mapManager.selectedConeIndex_ = -1;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Delete the selected cone.");

                ImGui::EndPopup();
            }

            mapManager.processPendingTextures();
            ImGui::End(); 
        }

        
        if (fileDialogActive.load()) {
            if (fileDialogFuture.valid()) { 
                if (fileDialogFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    selectedFilePath = fileDialogFuture.get();
                    fileDialogActive.store(false);
                    if (!selectedFilePath.empty()) {
                        printf("Selected file: %s\n", selectedFilePath.c_str());
                        std::filesystem::path filePath(selectedFilePath);
                        currentPath = filePath.parent_path().string();
                        
                        gpsManager.stop();
                        gps_interface_close(&gpsManager.getGPS());

                        if (gps_interface_open_file(&gpsManager.getGPS(), selectedFilePath.c_str()) == -1) {
                            printf("Error opening selected file: %s\n", selectedFilePath.c_str());
                            notificationManager.showPopup("FileBrowser", "Error", "Failed to open selected file.", NotificationType::Error);
                        } else {
                            gpsManager.resetSessionData();
                            gpsManager.start();

                            notificationManager.showPopup("Success", "Success", "Successfully loaded log file.", NotificationType::Success);
                        }
                    } else {
                        printf("User canceled file selection or an error occurred.\n");
                    }
                }
            } else {
                
                fileDialogActive.store(false);
            }
        }

        
        if (conesDialogActive.load()) {
            if (conesDialogFuture.valid()) { 
                if (conesDialogFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    selectedConesPath = conesDialogFuture.get();
                    conesDialogActive.store(false);
                    if (!selectedConesPath.empty()) {
                        printf("Selected CSV file: %s\n", selectedConesPath.c_str());

                        std::vector<cone_t> loadedCones;

                        std::filesystem::path filePath(selectedConesPath);
                        currentPath = filePath.parent_path().string();

                        
                        if (conesLoader.loadFromCSV(selectedConesPath, loadedCones)) {
                            
                            gpsManager.clearCones();

                            
                            for (const auto& cone : loadedCones) {
                                gpsManager.addCone(cone);
                            }

                            notificationManager.showPopup("Cones_Loaded", "Success", "Successfully loaded cones from CSV.", NotificationType::Success);
                            printf("Cones loaded from CSV: %s\n", selectedConesPath.c_str());
                            resetView = true; 
                        } else {
                            notificationManager.showPopup("Cones_Load_Error", "Error", "Failed to load cones from the selected CSV file.", NotificationType::Error);
                            printf("Failed to load cones from CSV: %s\n", selectedConesPath.c_str());
                        }
                    } else {
                        printf("User canceled CSV file selection or an error occurred.\n");
                    }
                }
            } else {
                
                conesDialogActive.store(false);
            }
        }


        
        gui.endFrame();
    }

    
    SaveConfig(currentTheme, fontManager.getSelectedFontIndex(), currentPath);
    
    
    gpsManager.stop();
    gui.cleanup();
    NFD_Quit();

    return 0;
}
