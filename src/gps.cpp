#include "gps.hpp"
#include "notifications.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <sys/stat.h>
#include "gps_interface.h"
#include "acr.h"
#include "defines.h"
#include "main.h"
#include "utils.h"

// Constructor
GPSManager::GPSManager(NotificationManager& notificationManager)
    : kill_thread_(false), saveCone_(false), conePlacementMode_(false),
      currentPosition_(0.0f, 0.0f), notificationManager_(notificationManager) {
    memset(&session_, 0, sizeof(full_session_t));
    memset(&cone_session_, 0, sizeof(cone_session_t));
    memset(&user_data_, 0, sizeof(user_data_t));
    user_data_.basepath = "";
    user_data_.cone = &cone_;
    user_data_.session = &session_;
    user_data_.cone_session = &cone_session_;
}

// Destructor
GPSManager::~GPSManager() {
    stop();
}

// Implement the getGPSData() method
gps_parsed_data_t GPSManager::getGPSData() const {
    std::lock_guard<std::mutex> lock(renderLock_);
    return gps_data_;
}

// Function to initialize GPS interface
int GPSManager::initialize(const char* port_or_file) {
    int res = 0;
    gps_interface_initialize(&gps_);
    if (port_or_file) {
        struct stat statbuf;
        if (stat(port_or_file, &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
            // It's a character device (serial port)
            char buff[255];
            snprintf(buff, sizeof(buff), "sudo chmod 777 %s", port_or_file);
            printf("Changing permissions on serial port: %s with command: %s\n",
                   port_or_file, buff);
            system(buff);
            res = gps_interface_open(&gps_, port_or_file, B230400);
        }
        else if (stat(port_or_file, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            // It's a regular file
            printf("Opening file: %s\n", port_or_file);
            res = gps_interface_open_file(&gps_, port_or_file);
        }
        else {
            printf("Error: %s does not exist or is not a regular file.\n", port_or_file);
            notificationManager_.showPopup("GPSManager", "Error", "Specified file does not exist.", NotificationType::Error);
        }
        if (res == -1) {
            printf("Error: GPS not found or failed to initialize.\n");
            notificationManager_.showPopup("GPSManager", "Error", "GPS not found or failed to initialize.", NotificationType::Error);
        }
    }
    return res;
}

void GPSManager::deleteCone(int index) {
    std::lock_guard<std::mutex> lock(renderLock_);
    if (index >= 0 && index < cones_.size()) {
        cones_.erase(cones_.begin() + index);
    }
}

// Start GPS reading thread
void GPSManager::start() {
    if (gpsThread_.joinable()) return;
    kill_thread_.store(false);
    gpsThread_ = std::thread(&GPSManager::readGPSLoop, this);
}

// Stop GPS reading thread
void GPSManager::stop() {
    kill_thread_.store(true);
    if (gpsThread_.joinable()) {
        gpsThread_.join();
    }
    gps_interface_close(&gps_);
}

// Reset session data
void GPSManager::resetSessionData() {
    std::lock_guard<std::mutex> lock(renderLock_);
    memset(&session_, 0, sizeof(full_session_t));
    memset(&cone_session_, 0, sizeof(cone_session_t));
    memset(&user_data_, 0, sizeof(user_data_t));
    user_data_.basepath = user_data_.basepath; // Retain basepath
    user_data_.cone = &cone_;
    user_data_.session = &session_;
    user_data_.cone_session = &cone_session_;
    trajectory_.clear();
    cones_.clear();
    currentPosition_ = ImVec2(0.0f, 0.0f); // Reset current GPS position
}

// Function to read GPS data in a separate thread
void GPSManager::readGPSLoop() {
    int fail_count = 0;
    int res = 0;
    unsigned char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
    char line[GPS_MAX_LINE_SIZE];
    while (!kill_thread_.load()) {
        int start_size, line_size;
        gps_protocol_type protocol;
        protocol = gps_interface_get_line(&gps_, start_sequence, &start_size, line, &line_size, true);
        if (protocol == GPS_PROTOCOL_TYPE_SIZE) {
            fail_count++;
            if (fail_count > 10) {
                printf("Error: GPS disconnected or unable to read data.\n");
                notificationManager_.showPopup("GPSManager_Error", "Error", "GPS disconnected or unable to read data.", NotificationType::Error);
                return;
            }
            continue;
        }
        else {
            fail_count = 0;
        }

        gps_protocol_and_message match;
        res = gps_match_message(&match, line, protocol);
        if (res == -1) {
            continue;
        }

        gps_parse_buffer(&gps_data_, &match, line, get_t());

        if (match.protocol == GPS_PROTOCOL_TYPE_UBX) {
            if (match.message == GPS_UBX_TYPE_NAV_HPPOSLLH) {
                std::lock_guard<std::mutex> lock(renderLock_);
                static double height = 0.0;

                if (CONE_ENABLE_MEAN && currentPosition_.x != 0.0 && currentPosition_.y != 0.0) {
                    currentPosition_.x = currentPosition_.x * CONE_MEAN_COMPLEMENTARY + gps_data_.hpposllh.lon * (1.0 - CONE_MEAN_COMPLEMENTARY);
                    currentPosition_.y = currentPosition_.y * CONE_MEAN_COMPLEMENTARY + gps_data_.hpposllh.lat * (1.0 - CONE_MEAN_COMPLEMENTARY);
                    height = height * CONE_MEAN_COMPLEMENTARY + gps_data_.hpposllh.height * (1.0 - CONE_MEAN_COMPLEMENTARY);
                }
                else {
                    currentPosition_.x = gps_data_.hpposllh.lon;
                    currentPosition_.y = gps_data_.hpposllh.lat;
                    height = gps_data_.hpposllh.height;
                }

                cone_.timestamp = gps_data_.hpposllh._timestamp;
                cone_.lon = currentPosition_.x;
                cone_.lat = currentPosition_.y;
                cone_.alt = height;

                static int count = 0;
                if (session_.active && count % 10 == 0) {
                    trajectory_.emplace_back(currentPosition_);
                    count = 0;
                }
                count++;
            }
        }

        if (session_.active) {
            gps_to_file(&session_.files, &gps_data_, &match);
        }

        if (saveCone_.load()) {
            saveCone_.store(false);
            cone_session_write(&cone_session_, &cone_);
            FILE *tmp = cone_session_.file;
            cone_session_.file = stdout;
            cone_session_write(&cone_session_, &cone_);
            cone_session_.file = tmp;
            cones_.push_back(cone_);
            //notificationManager_.showPopup("GPSManager", "Cone Saved", "A new cone has been saved.", NotificationType::Info);
        }
    }
}

// Retrieve current GPS position
ImVec2 GPSManager::getCurrentPosition() const {
    return currentPosition_;
}

// Retrieve trajectory
std::vector<ImVec2> GPSManager::getTrajectory() const {
    return trajectory_;
}

std::vector<cone_t>& GPSManager::getCones() {
    return cones_;
}

// Set cone placement mode
void GPSManager::setConePlacementMode(bool mode) {
    conePlacementMode_ = mode;
}

// Get cone placement mode
bool GPSManager::getConePlacementMode() const {
    return conePlacementMode_;
}

// Get GPS serial port
gps_serial_port& GPSManager::getGPS() {
    return gps_;
}

std::mutex& GPSManager::getRenderLock() {
    return renderLock_;
}

// Get cone session
cone_session_t& GPSManager::getConeSession() {
    return cone_session_;
}
void GPSManager::setConeId(cone_id id) {
    cone_.id = id;
}

// Get session
full_session_t& GPSManager::getSession() {
    return session_;
}
