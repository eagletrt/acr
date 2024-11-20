
#ifndef GPS_HPP
#define GPS_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include "imgui.h"  
#include "notifications.hpp" 
#include "config.hpp"

extern "C" {
    #include "gps_interface.h"
    #include "acr.h"
    #include "defines.h"
    #include "main.h"
    #include "utils.h"
}

class NotificationManager;

class GPSManager {
public:
    GPSManager(NotificationManager& notificationManager);
    ~GPSManager();

    
    int initialize(const char* port_or_file);

    
    void start();
    void stop();

    
    void resetSessionData();

    
    ImVec2 getCurrentPosition() const;
    std::vector<ImVec2> getTrajectory() const;
    std::vector<cone_t>& getCones();

    
    void setConePlacementMode(bool mode);
    bool getConePlacementMode() const;

    
    gps_serial_port& getGPS();
    cone_session_t& getConeSession();
    full_session_t& getSession();

    
    std::mutex& getRenderLock();

    
    gps_parsed_data_t getGPSData() const;

    
    std::atomic<bool> saveCone_;

    
    void deleteCone(int index);
    void setConeId(cone_id id);
    void clearCones();
    void addCone(const cone_t& cone);
    mutable std::mutex renderLock_;

private:
    std::atomic<bool> kill_thread_;
    bool conePlacementMode_;

    gps_serial_port gps_;
    gps_parsed_data_t gps_data_;
    cone_t cone_;
    user_data_t user_data_;
    full_session_t session_;
    cone_session_t cone_session_;

    ImVec2 currentPosition_;
    std::vector<ImVec2> trajectory_;
    std::vector<cone_t> cones_;

    std::thread gpsThread_;
    NotificationManager& notificationManager_;
    

    
    void readGPSLoop();
};

#endif 
