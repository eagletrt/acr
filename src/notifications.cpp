#include "notifications.hpp"
#include <imgui/imgui.h>
#include "icon_manager.hpp"

std::vector<ActiveNotification> activeNotifications;
std::mutex activeNotificationsMutex;

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

// Helper function to get icon based on notification type
ImTextureID getIconForNotification(NotificationType type) {
    switch (type) {
        case NotificationType::Success:
            return getIconTexture("Success");
        case NotificationType::Error:
            return getIconTexture("Error");
        case NotificationType::Info:
        default:
            return getIconTexture("Info");
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
        ImTextureID iconTexture = getIconForNotification(it->type);

        switch (it->type) {
            case NotificationType::Success:
                textColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                break;
            case NotificationType::Error:
                textColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                break;
            case NotificationType::Info:
            default:
                textColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
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
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoCollapse;

        // Unique window name
        std::string windowName = "Notification_" + it->source + "_" + std::to_string(notificationIndex);

        if (ImGui::Begin(windowName.c_str(), nullptr, window_flags)) {
            // Notification icon
            if (iconTexture != 0) {
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