#define STB_IMAGE_IMPLEMENTATION
#include "viewer.hpp"
#include <GLFW/glfw3.h>
#include <string.h>
#include <atomic>
#include <algorithm> // For std::sort
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <queue> // Include <queue>
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

// ----------------------- Global Variables -----------------------

std::mutex renderLock;
std::atomic<bool> kill_thread(false);
std::atomic<bool> save_cone(false);
bool conePlacementMode = false; // Indicates if we are in cone placement mode

gps_serial_port gps;
gps_parsed_data_t gps_data;
cone_t cone;
user_data_t user_data;
full_session_t session;
cone_session_t cone_session;

ImVec2 currentPosition;
std::vector<ImVec2> trajectory;
std::vector<cone_t> cones;

// Map Boundaries
ImVec2 povoBoundBL{11.1484815430000008, 46.0658863580000002};
ImVec2 povoBoundTR{11.1515535430000003, 46.0689583580000033};
ImVec2 vadenaBoundBL{11.3097566090000008, 46.4300119620000018};
ImVec2 vadenaBoundTR{11.3169246090000009, 46.4382039620000029};
ImVec2 fsgBoundBL{8.558931763076039, 49.32344089057215};
ImVec2 fsgBoundTR{8.595726757113576, 49.335430701657735};
ImVec2 alaBoundBL{11.010747212958533, 45.784567764275764};
ImVec2 alaBoundTR{11.013506837511347, 45.78713363420464};
ImVec2 varanoBoundBL{10.013347232876077, 44.67756187921092};
ImVec2 varanoBoundTR{10.031744729894845, 44.684102509035036};

// File Browser State
bool showFileBrowser = true; // Set to true to open automatically
std::string currentPath = "acr"; // Set initial directory to 'acr'
std::vector<std::filesystem::directory_entry> entries;
std::string selectedFile = "";

// GPS Thread
std::thread gpsThread;

// Font Control Variables
struct FontInfo {
    std::string name;
    ImFont* font;
};
std::vector<FontInfo> availableFonts;
int selectedFontIndex = 0; // Selected font index
float fontScale = 1.0f; // Initial font scale

// Icon Control Variables
struct IconInfo {
    std::string name;
    ImTextureID texture;
    ImVec2 size;
};
std::vector<IconInfo> icons;

// Active Notifications
enum class NotificationType {
    Info,
    Success,
    Error
};

// Updated ActiveNotification Structure
struct ActiveNotification {
    std::string source;          // Unique identifier for the notification
    std::string title;           // Notification title
    std::string message;         // Notification message
    NotificationType type;       // Type of notification
    float displayDuration;       // Duration in seconds
    float elapsedTime;           // Elapsed time in seconds
};

struct MapInfo {
    std::string name;      // Name of the map
    std::string filePath;  // Path to the image file
    ImVec2 boundBL;        // Bottom-left boundary coordinates
    ImVec2 boundTR;        // Top-right boundary coordinates
    ImTextureID texture;   // Loaded texture ID
};

std::vector<MapInfo> maps;
int selectedMapIndex = 0; // Index of the selected map

std::vector<ActiveNotification> activeNotifications;
std::mutex activeNotificationsMutex;

// Selected Cone for Context Menu and Dragging
int selectedConeIndex = -1;
bool showConeContextMenu = false;

// Dragging State
bool isDraggingCone = false;
int draggingConeIndex = -1;

// Function Declarations
void readGPSLoop();
ImTextureID loadImageJPG(const char *path);
ImTextureID loadImagePNG(const char *path); // Function to load PNG icons
GLFWwindow *setupImGui();
void startFrame();
void endFrame(GLFWwindow *window);
void renderFileBrowser();
void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir);
void loadIcons(); // Function to load icons
void processNotifications(float deltaTime);
void showPopup(const std::string& source, const std::string& title, const std::string& message, NotificationType type);
int findClosestCone(const ImPlotPoint& mousePos, const std::vector<cone_t>& cones, float hitRadius);

// ----------------------- Function Definitions -----------------------

// Function to get Desktop path
std::string getDesktopPath() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    if (home) {
        return std::string(home) + "\\Desktop";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Desktop";
    }
#endif
    // Fallback to current directory if HOME is not found
    return ".";
}

// Function to load multiple fonts from the fonts directory
void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir) {
    // Supported font file extensions
    std::vector<std::string> extensions = { ".ttf", ".otf" };

    for (const auto& entry : std::filesystem::directory_iterator(fontsDir)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();
            // Check if the file has a supported font extension
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                // Extract font name from filename
                std::string name = entry.path().stem().string();
                // Load font with default size
                ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
                if (font) {
                    availableFonts.push_back(FontInfo{ name, font });
                    printf("Loaded font: %s\n", name.c_str());
                }
                else {
                    printf("Failed to load font: %s\n", path.c_str());
                }
            }
        }
    }

    // If no fonts were loaded, load the default font
    if (availableFonts.empty()) {
        availableFonts.push_back(FontInfo{ "Default", io.Fonts->AddFontDefault() });
        printf("No custom fonts found. Loaded default font.\n");
    }
}

// Function to load an image and create an OpenGL texture (JPEG)
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

// Function to load an image and create an OpenGL texture (PNG)
ImTextureID loadImagePNG(const char* path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (data == NULL) {
        printf("Error loading image: %s\n", path);
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload image data to the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return (ImTextureID)(intptr_t)tex;
}

// Function to set up a custom ImGui theme
void setCustomTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Base colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_MenuBarBg]            = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Separator]            = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.40f, 0.43f, 0.47f, 1.00f);
    colors[ImGuiCol_SeparatorActive]      = ImVec4(0.50f, 0.53f, 0.57f, 1.00f);
    colors[ImGuiCol_Text]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Adjust style variables
    style.WindowRounding    = 5.0f;
    style.FrameRounding     = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding      = 5.0f;
    style.FramePadding      = ImVec2(5.0f, 5.0f);
    style.ItemSpacing       = ImVec2(10.0f, 10.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 15.0f;
    style.WindowPadding     = ImVec2(10.0f, 10.0f);
}

// Function to load icons
void loadIcons() {
    // Example: Load an information icon
    ImTextureID infoIcon = loadImagePNG("icons/info.png"); // Ensure the path is correct
    if (infoIcon != 0) {
        icons.push_back({ "Info", infoIcon, ImVec2(16, 16) });
        printf("Loaded icon: Info\n");
    } else {
        printf("Failed to load icon: info.png\n");
    }

    // Example: Load a success icon
    ImTextureID successIcon = loadImagePNG("icons/success.png"); // Ensure the path is correct
    if (successIcon != 0) {
        icons.push_back({ "Success", successIcon, ImVec2(16, 16) });
        printf("Loaded icon: Success\n");
    } else {
        printf("Failed to load icon: success.png\n");
    }

    // Example: Load an error icon
    ImTextureID errorIcon = loadImagePNG("icons/error.png"); // Ensure the path is correct
    if (errorIcon != 0) {
        icons.push_back({ "Error", errorIcon, ImVec2(16, 16) });
        printf("Loaded icon: Error\n");
    } else {
        printf("Failed to load icon: error.png\n");
    }

    // Add more icons as needed
}

// Function to set up ImGui and create a GLFW window
GLFWwindow *setupImGui() {
    if (!glfwInit())
        return nullptr;

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(1280, 720, "ACR", nullptr, nullptr);
    if (window == nullptr)
        return nullptr;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Apply custom theme
    setCustomTheme();

    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Load fonts from the 'fonts' directory
    loadFontsFromDirectory(io, "../assets/fonts/");

    // Set the default font
    if (!availableFonts.empty()) {
        io.FontDefault = availableFonts[selectedFontIndex].font;
    }

    return window;
}

// Function to start a new ImGui frame
void startFrame() {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// Function to render the ImGui frame and swap buffers
void endFrame(GLFWwindow *window) {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.10f, 0.10f, 1.00f); // Dark background
    glClear(GL_COLOR_BUFFER_BIT);

    // Render ImGui Draw Data
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}

// Function to show a popup notification
void showPopup(const std::string& source, const std::string& title, const std::string& message, NotificationType type) {
    std::lock_guard<std::mutex> lock(activeNotificationsMutex);
    // Check if a notification with the same source already exists
    bool found = false;
    for (auto& notif : activeNotifications) {
        if (notif.source == source) {
            notif.elapsedTime = 0.0f; // Reset elapsed time
            found = true;
            break;
        }
    }
    if (!found) {
        // Remove all existing notifications
        activeNotifications.clear();
        // Add the new notification with all members initialized
        activeNotifications.emplace_back(ActiveNotification{ source, title, message, type, 2.0f, 0.0f });
    }
}

// Function to process and render active notifications
void processNotifications(float deltaTime) {
    std::lock_guard<std::mutex> lock(activeNotificationsMutex);
    int notificationIndex = 0;
    const int MAX_NOTIFICATIONS = 5; // Limit the maximum number of visible notifications

    // Remove excess notifications
    if (activeNotifications.size() > MAX_NOTIFICATIONS) {
        activeNotifications.erase(activeNotifications.begin(), activeNotifications.begin() + (activeNotifications.size() - MAX_NOTIFICATIONS));
    }

    // Get the main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 viewportPos = viewport->Pos;
    ImVec2 viewportSize = viewport->Size;

    float padding = 10.0f;
    float notificationWidth = 300.0f;
    float notificationHeight = 60.0f;

    for (auto it = activeNotifications.begin(); it != activeNotifications.end(); ) {
        it->elapsedTime += deltaTime;
        if (it->elapsedTime >= it->displayDuration) {
            it = activeNotifications.erase(it);
            continue;
        }

        // Calculate transparency for fade-in and fade-out effect
        float alpha = 0.8f;
        float fadeInTime = 0.5f;
        float fadeOutTime = 0.5f;

        if (it->elapsedTime < fadeInTime) {
            alpha *= (it->elapsedTime / fadeInTime);
        } else if (it->displayDuration - it->elapsedTime < fadeOutTime) {
            alpha *= ((it->displayDuration - it->elapsedTime) / fadeOutTime);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, alpha));

        // Determine colors and icons based on notification type
        ImVec4 textColor;
        ImTextureID iconTexture = 0;
        switch (it->type) {
            case NotificationType::Success:
                textColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                for (const auto& icon : icons) {
                    if (icon.name == "Success") {
                        iconTexture = icon.texture;
                        break;
                    }
                }
                break;
            case NotificationType::Error:
                textColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                for (const auto& icon : icons) {
                    if (icon.name == "Error") {
                        iconTexture = icon.texture;
                        break;
                    }
                }
                break;
            case NotificationType::Info:
            default:
                textColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
                for (const auto& icon : icons) {
                    if (icon.name == "Info") {
                        iconTexture = icon.texture;
                        break;
                    }
                }
                break;
        }

        // Define the position of the notification
        ImVec2 windowPos = ImVec2(
            viewportPos.x + viewportSize.x - notificationWidth - padding,
            viewportPos.y + viewportSize.y - notificationHeight - padding - (notificationHeight + padding) * notificationIndex
        );

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(notificationWidth, notificationHeight));

        // Define window flags
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_NoInputs;

        // Unique window name
        std::string windowName = "Notification_" + it->source + "_" + std::to_string(notificationIndex);

        if (ImGui::Begin(windowName.c_str(), nullptr, window_flags)) {
            // Notification icon
            if (iconTexture != 0 && !icons.empty()) {
                ImGui::Image(iconTexture, ImVec2(24, 24));
                ImGui::SameLine();
            }

            // Notification title
            ImGui::TextColored(textColor, "%s", it->title.c_str());
            ImGui::Separator();
            // Notification message
            ImGui::TextWrapped("%s", it->message.c_str());
        }
        ImGui::End();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ++it;
        ++notificationIndex;
    }
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

            selectedFile.clear();
            showFileBrowser = false; // Close the file browser
        }

        // Tooltip for Load button
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to load the selected log file.");
        }
    }

    ImGui::End();
}

// Function to read GPS data in a separate thread
void readGPSLoop() {
    int fail_count = 0;
    int res = 0;
    unsigned char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
    char line[GPS_MAX_LINE_SIZE];
    while (!kill_thread.load()) {
        int start_size, line_size;
        gps_protocol_type protocol;
        protocol = gps_interface_get_line(&gps, start_sequence, &start_size, line, &line_size, true);
        if (protocol == GPS_PROTOCOL_TYPE_SIZE) {
            fail_count++;
            if (fail_count > 10) {
                printf("Error: GPS disconnected or unable to read data.\n");
                showPopup("GPS_Error", "Error", "GPS disconnected or unable to read data.", NotificationType::Error);
                return;
            }
            continue;
        }
        else {
            fail_count = 0;
        }

        gps_protocol_and_message match;
        res = gps_match_message(&match, line, protocol);
        if (res == -1) {
            continue;
        }

        gps_parse_buffer(&gps_data, &match, line, get_t());

        if (match.protocol == GPS_PROTOCOL_TYPE_UBX) {
            if (match.message == GPS_UBX_TYPE_NAV_HPPOSLLH) {
                std::unique_lock<std::mutex> lck(renderLock);
                static double height = 0.0;

                if (CONE_ENABLE_MEAN && currentPosition.x != 0.0 && currentPosition.y != 0.0) {
                    currentPosition.x = currentPosition.x * CONE_MEAN_COMPLEMENTARY + gps_data.hpposllh.lon * (1.0 - CONE_MEAN_COMPLEMENTARY);
                    currentPosition.y = currentPosition.y * CONE_MEAN_COMPLEMENTARY + gps_data.hpposllh.lat * (1.0 - CONE_MEAN_COMPLEMENTARY);
                    height = height * CONE_MEAN_COMPLEMENTARY + gps_data.hpposllh.height * (1.0 - CONE_MEAN_COMPLEMENTARY);
                }
                else {
                    currentPosition.x = gps_data.hpposllh.lon;
                    currentPosition.y = gps_data.hpposllh.lat;
                    height = gps_data.hpposllh.height;
                }

                cone.timestamp = gps_data.hpposllh._timestamp;
                cone.lon = currentPosition.x;
                cone.lat = currentPosition.y;
                cone.alt = height;

                static int count = 0;
                if (session.active && count % 10 == 0) {
                    trajectory.push_back(currentPosition);
                    count = 0;
                }
                count++;
            }
        }

        if (session.active) {
            gps_to_file(&session.files, &gps_data, &match);
        }

        if (save_cone.load()) {
            save_cone.store(false);
            cone_session_write(&cone_session, &cone);
            // Print to stdout
            FILE *tmp = cone_session.file;
            cone_session.file = stdout;
            cone_session_write(&cone_session, &cone);
            cone_session.file = tmp;
            cones.push_back(cone);
        }
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

    return closestIndex;  // Return the index of the closest cone, or -1 if none is close enough
}

int main(int argc, char **argv) {
    const char *port_or_file = nullptr;
    if (argc == 2) {
        port_or_file = argv[1];
    }
    NFD_Init();

    // Set initial directory to Desktop
    currentPath = getDesktopPath();

    const char *basepath = getenv("HOME");

    GLFWwindow *window = setupImGui();
    if (window == nullptr) {
        printf("Failed to initialize GLFW or create window.\n");
        return -1;
    }

    // Load icons
    loadIcons();

    memset(&session, 0, sizeof(full_session_t));
    memset(&cone_session, 0, sizeof(cone_session_t));
    memset(&user_data, 0, sizeof(user_data_t));
    user_data.basepath = currentPath.c_str(); // Updated to use currentPath
    user_data.cone = &cone;
    user_data.session = &session;
    user_data.cone_session = &cone_session;

    int res = 0;
    gps_interface_initialize(&gps);
    if (port_or_file) {
        if (std::filesystem::is_character_file(port_or_file)) {
            char buff[255];
            snprintf(buff, 255, "sudo chmod 777 %s", port_or_file);
            printf("Changing permissions on serial port: %s with command: %s\n",
                   port_or_file, buff);
            system(buff);
            res = gps_interface_open(&gps, port_or_file, B230400);
        }
        else if (std::filesystem::is_regular_file(port_or_file)) {
            printf("Opening file: %s\n", port_or_file);
            res = gps_interface_open_file(&gps, port_or_file);  // No need to call .c_str() on const char*
        }
        else {
            printf("Error: %s does not exist.\n", port_or_file);
            showPopup("FileBrowser", "Error", "Specified file does not exist.", NotificationType::Error);
            // Show the file browser if the specified file does not exist
            showFileBrowser = true;
        }
        if (res == -1) {
            printf("Error: GPS not found or failed to initialize.\n");
            showPopup("GPS_Error", "Error", "GPS not found or failed to initialize.", NotificationType::Error);
            // Show the file browser in case of initialization failure
            showFileBrowser = true;
        }
        else if (port_or_file && std::filesystem::is_regular_file(port_or_file)) {
            // Start the GPS thread only if a file was successfully opened
            gpsThread = std::thread(readGPSLoop);
        }
    }   

    // Adding maps
    maps.push_back({"Povo", "../assets/Povo.jpg", povoBoundBL, povoBoundTR, 0});
    maps.push_back({"Vadena", "../assets/Vadena.jpg", vadenaBoundBL, vadenaBoundTR, 0});
    maps.push_back({"FSG", "../assets/FSG.jpg", fsgBoundBL, fsgBoundTR, 0});
    maps.push_back({"Ala", "../assets/Ala.jpg", alaBoundBL, alaBoundTR, 0});
    maps.push_back({"Varano", "../assets/Varano.jpg", varanoBoundBL, varanoBoundTR, 0});

    // Load textures for each map
    for (auto& map : maps) {
        map.texture = loadImageJPG(map.filePath.c_str());
        if (map.texture == 0) {
            printf("Error loading map image: %s\n", map.filePath.c_str());
            showPopup("Map_Error", "Error", "Unable to load map: " + map.name + "", NotificationType::Error);
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

    float mapOpacity = 0.5f;

    // Initialize time for deltaTime calculation
    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        startFrame();

        // Process active notifications
        processNotifications(deltaTime);

        ImGui::Begin("ACR");

        // Button to open the file browser
        if (ImGui::Button("Load Log File")) {
            // Define file filters using the correct format
            nfdu8filteritem_t filterList[] = {
                { "Log files", "log,txt" }  // {"Description", "Extensions separated by commas"}
            };
            size_t filterCount = sizeof(filterList) / sizeof(filterList[0]);

            // Initialize dialog arguments
            nfdopendialogu8args_t args = {0};
            args.filterList = filterList;
            args.filterCount = filterCount;
            args.defaultPath = NULL; // You can specify a default path if desired

            // Correct declaration of outPath
            nfdu8char_t* outPath = nullptr;

            // Call the file dialog with the correct parameters
            nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

            if (result == NFD_OKAY && outPath != nullptr) {
                printf("Selected file: %s\n", outPath);

                // Stop the existing GPS thread
                kill_thread.store(true);
                if (gpsThread.joinable()) {
                    gpsThread.join();
                }

                // Close the previous GPS interface
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

                    // Show a success notification
                    showPopup("Success", "Success", "Successfully loaded log file.", NotificationType::Success);
                }

                // Free the file path returned by NFD
                NFD_FreePathU8(outPath);
            }
            else if (result == NFD_CANCEL) {
                printf("User canceled file selection.\n");
            }
            else {
                printf("Error: %s\n", NFD_GetError());
            }
        }

        // Settings Section using Tabs
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
        // Map Opacity Slider with Tooltip
        ImGui::BeginGroup();
        ImGui::Text("Map Opacity");
        ImGui::SameLine();
        ImGui::SliderFloat("##MapOpacity", &mapOpacity, 0.0f, 1.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Adjust the opacity of the map overlay.");
        ImGui::EndGroup();

        // Map Selection Radio Buttons with Tooltip
        ImGui::Text("Select Map:");
        for (int i = 0; i < static_cast<int>(maps.size()); i++) {
            if (ImGui::RadioButton(maps[i].name.c_str(), &selectedMapIndex, i)) {
                // Show a confirmation popup when a map is selected
                showPopup("Map", "Map Selected", "You have selected the map: " + maps[i].name, NotificationType::Success);
                printf("Selected map: %s\n", maps[i].name.c_str());
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Select the map to display.");
        }

        // Font Selection Dropdown with Tooltip
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

        // Slider for font scale with Tooltip
        ImGui::BeginGroup();
        ImGui::Text("Font Scale");
        ImGui::SameLine();
        ImGui::SliderFloat("##FontScale", &fontScale, 0.5f, 2.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Adjust the global font scale.");
        ImGui::EndGroup();

        ImGui::Text("Font Scale: %.2f", fontScale);

        // Apply global font scale
        ImGui::GetIO().FontGlobalScale = fontScale;

        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
}


        ImGui::Separator();

        // Display HDOP with Tooltip
        ImGui::Text("HDOP: %.2f [m]", gps_data.hpposllh.hAcc);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Horizontal Dilution of Precision (HDOP) indicates the GPS accuracy.");

        {
            std::unique_lock<std::mutex> lck(renderLock);
            // Handle key presses with Tooltips
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

        // Plotting GPS Data
        ImVec2 size = ImGui::GetContentRegionAvail();

        if (ImPlot::BeginPlot("GpsPositions", size, ImPlotFlags_Equal | ImPlotFlags_NoTitle)) {
            const MapInfo& selectedMap = maps[selectedMapIndex];

            // Set axis limits based on the selected map's boundaries
            ImPlot::SetupAxesLimits(selectedMap.boundBL.x, selectedMap.boundTR.x,
                            selectedMap.boundBL.y, selectedMap.boundTR.y, ImGuiCond_Always);

            // Customize plot style
            ImPlot::GetStyle().LineWeight = 2.0f;
            ImPlot::GetStyle().MarkerSize = 6.0f;

            // Plot Map Image
            if (selectedMap.texture != 0) {
                ImPlot::PlotImage(selectedMap.name.c_str(), 
                                  selectedMap.texture, 
                                  selectedMap.boundBL, selectedMap.boundTR,
                                  ImVec2(0, 0), ImVec2(1, 1), 
                                  ImVec4(1, 1, 1, mapOpacity));
            }

            // Plot Trajectory
            if (!trajectory.empty()) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                ImPlot::PlotScatter("Trajectory", &trajectory[0].x, &trajectory[0].y,
                                    static_cast<int>(trajectory.size()), 0, 0, sizeof(ImVec2));
            }

            // Plot Cones and Handle Right-Click Detection
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
                float hitRadius = 0.001f;  // Adjust this value based on plot scale

                // Find the closest cone to the mouse position
                int closestConeIndex = findClosestCone(mousePos, cones, hitRadius);

                if (closestConeIndex != -1) {
                    selectedConeIndex = closestConeIndex;  // Select the closest cone
                    showConeContextMenu = true;  // Show context menu for the selected cone
                }
            }

            // Plot Current GPS Position only if it's valid
            if (currentPosition.x != 0.0f || currentPosition.y != 0.0f) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 10, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 0.0);
                ImPlot::PlotScatter("Current", &currentPosition.x, &currentPosition.y, 1);
            }

            // Positioning the cone with a click on the map
            if (conePlacementMode && ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // Get the mouse position in plot coordinates
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

        // Handle the Context Menu Popup
        if (showConeContextMenu && selectedConeIndex >= 0 && selectedConeIndex < static_cast<int>(cones.size())) {
            ImGui::OpenPopup("Cone Menu");
            showConeContextMenu = false;
        }

        if (ImGui::BeginPopup("Cone Menu")) {
            // Display icon based on cone type
            ImTextureID coneIcon = 0;
            switch (cones[selectedConeIndex].id) {
                case CONE_ID_ORANGE:
                    for (const auto& icon : icons) {
                        if (icon.name == "Success") { // Assuming "Success" icon represents orange
                            coneIcon = icon.texture;
                            break;
                        }
                    }
                    break;
                case CONE_ID_YELLOW:
                    for (const auto& icon : icons) {
                        if (icon.name == "Info") { // Assuming "Info" icon represents yellow
                            coneIcon = icon.texture;
                            break;
                        }
                    }
                    break;
                case CONE_ID_BLUE:
                    for (const auto& icon : icons) {
                        if (icon.name == "Error") { // Assuming "Error" icon represents blue
                            coneIcon = icon.texture;
                            break;
                        }
                    }
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

            // Dropdown to select new color with Tooltip
            const char* color_options[] = { "Orange", "Yellow", "Blue" };
            static int selected_color = 0;

            // Initialize selected_color based on current cone's id
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

            // Button to delete the cone with Tooltip
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

            // Button to start dragging the cone with Tooltip
            if (ImGui::Button("Move Cone")) {
                conePlacementMode = true;  // Activate cone placement mode
                ImGui::CloseCurrentPopup();
                showPopup("Move_Cone", "Move Cone", "Click on the map to move the cone to the desired position.", NotificationType::Info);
                printf("Cone placement mode activated.\n");
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Move the selected cone to a new position.");

            ImGui::EndPopup();
        }

        ImGui::End();

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

    // Cleanup ImGui and GLFW
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
