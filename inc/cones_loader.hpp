#ifndef CONES_LOADER_HPP
#define CONES_LOADER_HPP

#include <string>
#include <vector>
#include <mutex>
#include "map.hpp"  
#include "notifications.hpp" 

class ConesLoader {
public:
    ConesLoader(NotificationManager& notificationManager);
    ~ConesLoader();

    // Load cones from a CSV file
    bool loadFromCSV(const std::string& filePath);

    // Clear all loaded cones
    void clearCones();

    // Retrieve the loaded cones
    std::vector<cone_t> getCones() const;

private:
    std::vector<cone_t> cones_;
    mutable std::mutex conesMutex_;

    // Reference to NotificationManager
    NotificationManager& notificationManager_;
};

#endif // CONES_LOADER_HPP
