#ifndef FILE_BROWSER_HPP
#define FILE_BROWSER_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include "imgui.h"
#include "notifications.hpp"
#include "config.hpp"
#include "nfd.h"

class NotificationManager;

class FileBrowser {
public:
    FileBrowser(NotificationManager& notificationManager);
    ~FileBrowser();

    
    void initialize();

    
    void openFileDialog();

    
    void render();

    
    std::string getSelectedFile() const;

private:
    
    void fileDialogThreadFunction();

    bool showFileBrowser_;                     
    std::string currentPath_;                  
    std::vector<std::filesystem::directory_entry> entries_; 
    std::string selectedFile_;                 

    NotificationManager& notificationManager_;  

    
    std::thread fileDialogThread_;             
    std::mutex selectedFileMutex_;             
    std::atomic<bool> dialogActive_;           
    std::future<std::string> fileFuture_;      
    std::promise<std::string> filePromise_;    

    
    static inline std::string trim(const std::string& s);
};

#endif 
