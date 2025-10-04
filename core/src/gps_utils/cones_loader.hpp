#ifndef CONES_LOADER_HPP
#define CONES_LOADER_HPP

#include <string>
#include <vector>
#include "map.hpp"  
#include "notifications.hpp"
#include "config.hpp"

class NotificationManager;

class ConesLoader {
public:
    ConesLoader(NotificationManager& notificationManager);
    ~ConesLoader();

    bool loadFromCSV(const std::string& filePath, std::vector<cone_t>& loadedCones);

private:
    NotificationManager& notificationManager_;
};

#endif 
