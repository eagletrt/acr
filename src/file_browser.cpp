#include "file_browser.hpp"
#include "nfd.h"
#include "notifications.hpp"
#include "gps.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstdio>

// Constructor
FileBrowser::FileBrowser(NotificationManager& notificationManager)
    : showFileBrowser_(false), currentPath_(""), selectedFile_(""), notificationManager_(notificationManager) {}

// Destructor
FileBrowser::~FileBrowser() {}

void FileBrowser::initialize() {
    showFileBrowser_ = true; 
    currentPath_ = Utils::getDesktopPath(); // Initialize to the desktop path
    entries_.clear();
    selectedFile_.clear();
}
                                                                                
void FileBrowser::render() {
    if (!showFileBrowser_) return;

    ImGui::Begin("File Browser", &showFileBrowser_, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Current Path: %s", currentPath_.c_str());

    if (currentPath_ != std::filesystem::path(currentPath_).root_path()) {
        if (ImGui::Button("..")) {
            currentPath_ = std::filesystem::path(currentPath_).parent_path().string();
        }
        ImGui::SameLine();
    }

    entries_.clear();
    try {
        for (const auto& entry : std::filesystem::directory_iterator(currentPath_)) {
            entries_.emplace_back(entry);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Error accessing directory.");
        notificationManager_.showPopup("FileBrowser_Error", "Error", "Unable to access directory.", NotificationType::Error);
    }

    std::sort(entries_.begin(), entries_.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
        if (a.is_directory() && !b.is_directory()) return true;
        if (!a.is_directory() && b.is_directory()) return false;
        return a.path().filename().string() < b.path().filename().string();
    });

    for (const auto& entry : entries_) {
        const auto& path = entry.path();
        std::string name = path.filename().string();
        if (entry.is_directory()) {
            if (ImGui::Selectable((std::string("[") + name + "]").c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
                currentPath_ = path.string();
            }
        }
        else {
            if (ImGui::Selectable(name.c_str())) {
                if (path.extension() == ".log" || path.extension() == ".txt") {
                    selectedFile_ = path.string();
                }
            }
        }
    }

    if (!selectedFile_.empty()) {
        if (ImGui::Button("Load")) {
            printf("Selected file: %s\n", selectedFile_.c_str());


            notificationManager_.showPopup("FileBrowser_Load", "Success", "Successfully loaded log file.", NotificationType::Success);

            selectedFile_.clear();
            showFileBrowser_ = false;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to load the selected log file.");
        }
    }

    ImGui::End();
}

std::string FileBrowser::getSelectedFile() const {
    return selectedFile_;
}
