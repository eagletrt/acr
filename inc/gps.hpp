#ifndef GPS_HPP
#define GPS_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include "imgui.h"  
#include "notifications.hpp" 

extern "C" {
    #include "gps_interface.h"
    #include "acr.h"
    #include "defines.h"
    #include "main.h"
    #include "utils.h"
}

class GPSManager {
public:
    GPSManager(NotificationManager& notificationManager);
    ~GPSManager();

    // Initialize GPS interface
    int initialize(const char* port_or_file);

    // Start and stop GPS reading thread
    void start();
    void stop();

    // Reset session data
    void resetSessionData();

    // Getters for data
    ImVec2 getCurrentPosition() const;
    std::vector<ImVec2> getTrajectory() const;
    std::vector<cone_t>& getCones();

    // Setter and getter for cone placement mode
    void setConePlacementMode(bool mode);
    bool getConePlacementMode() const;

    // Getters for internal structures
    gps_serial_port& getGPS();
    cone_session_t& getConeSession();
    full_session_t& getSession();

    // Mutex accessor
    std::mutex& getRenderLock();

    // Retrieve parsed GPS data
    gps_parsed_data_t getGPSData() const;

    // Public atomic variable to trigger cone saving
    std::atomic<bool> saveCone_;

    // Function to delete a cone
    void deleteCone(int index);
    void setConeId(cone_id id);

private:
    std::atomic<bool> kill_thread_;
    bool conePlacementMode_;

    gps_serial_port gps_;
    gps_parsed_data_t gps_data_;
    cone_t cone_;
    user_data_t user_data_;
    full_session_t session_;
    cone_session_t cone_session_;
    mutable std::mutex renderLock_; // Single declaration

    ImVec2 currentPosition_;
    std::vector<ImVec2> trajectory_;
    std::vector<cone_t> cones_;

    std::thread gpsThread_;
    NotificationManager& notificationManager_;

    // Internal thread function
    void readGPSLoop();
};

#endif // GPS_HPP
