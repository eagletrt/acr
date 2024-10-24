#ifndef NOTIFICATIONS_HPP
#define NOTIFICATIONS_HPP

#include <string>
#include <vector>
#include <mutex>
#include "imgui.h"

class IconManager;

enum class NotificationType {
    Info,
    Success,
    Error
};

struct ActiveNotification {
    std::string source;         
    std::string title;           
    std::string message;        
    NotificationType type;     
    float displayDuration;      
    float elapsedTime;          
};

class NotificationManager {
public:
    // Constructor and Destructor
    NotificationManager(IconManager* iconManager);
    ~NotificationManager();

    // Show a popup notification
    void showPopup(const std::string& source, const std::string& title, const std::string& message, NotificationType type);

    // Process and render active notifications
    void processNotifications(float deltaTime);

private:
    std::vector<ActiveNotification> activeNotifications_;
    std::mutex activeNotificationsMutex_;
    IconManager* iconManager_;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
    ImTextureID getIconForNotification(NotificationType type) const;
};

#endif // NOTIFICATIONS_HPP
