#ifndef FILE_BROWSER_HPP
#define FILE_BROWSER_HPP

#include <string>
#include <vector>
#include <filesystem>
#include "imgui.h"
#include "notifications.hpp"

class FileBrowser {
public:
    FileBrowser(NotificationManager& notificationManager);
    ~FileBrowser();

    void initialize();

    void render();

    std::string getSelectedFile() const;

private:
    bool showFileBrowser_;
    std::string currentPath_;
    std::vector<std::filesystem::directory_entry> entries_;
    std::string selectedFile_;

    NotificationManager& notificationManager_;
};

#endif 
