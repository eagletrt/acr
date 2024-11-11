
#ifndef CONES_LOADER_HPP
#define CONES_LOADER_HPP

#include <string>
#include <vector>
#include <mutex>
#include "map.hpp"  
#include "notifications.hpp"
#include "config.hpp"


class NotificationManager;


struct ConeWithDescription {
    cone_t cone;
    std::string description;
};

class ConesLoader {
public:
    
    ConesLoader(NotificationManager& notificationManager);
    ~ConesLoader();

    
    bool loadFromCSV(const std::string& filePath);

    
    void clearCones();

    
    std::vector<ConeWithDescription> getConesWithDescriptions() const;

private:
    std::vector<ConeWithDescription> conesWithDescriptions_;
    mutable std::mutex conesMutex_;

    
    NotificationManager& notificationManager_;
};

#endif 
