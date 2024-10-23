#ifndef NOTIFICATIONS_HPP
#define NOTIFICATIONS_HPP

#include <string>
#include <vector>
#include <mutex>
#include "imgui/imgui.h"

enum class NotificationType {
    Info,
    Success,
    Error
};

// Structure for active notifications
struct ActiveNotification {
    std::string source;          // Unique identifier for the notification
    std::string title;           // Notification title
    std::string message;         // Notification message
    NotificationType type;       // Type of notification
    float displayDuration;       // Duration in seconds
    float elapsedTime;           // Elapsed time in seconds
};

// Function to show a popup notification
void showPopup(const std::string& source, const std::string& title, const std::string& message, NotificationType type);

// Function to process and render active notifications
void processNotifications(float deltaTime);

#endif