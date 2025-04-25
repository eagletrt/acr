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
#include "config.hpp"


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


GPSManager::~GPSManager() {
    stop();
}

gps_parsed_data_t GPSManager::getGPSData() const {
    std::lock_guard<std::mutex> lock(renderLock_);
    return gps_data_;
}


int GPSManager::initialize(const char* port_or_file) {
    int res = 0;
    gps_interface_initialize(&gps_);
    if (port_or_file) {
        struct stat statbuf;
        if (stat(port_or_file, &statbuf) == 0) {
            if (S_ISCHR(statbuf.st_mode)) {
                res = gps_interface_open(&gps_, port_or_file, GPS_DEFAULT_BAUDRATE);
                if (res == -1) {
                    printf("Error: failed to open serial port %s\n", port_or_file);
                    notificationManager_.showPopup(
                        "GPSManager",
                        "Error",
                        "Failed to open serial port. Check permissions or udev rules.",
                        NotificationType::Error
                    );
                }
            }
            else if (S_ISREG(statbuf.st_mode)) {
                printf("Opening file: %s\n", port_or_file);
                res = gps_interface_open_file(&gps_, port_or_file);
                if (res == -1) {
                    printf("Error: failed to open file %s\n", port_or_file);
                    notificationManager_.showPopup(
                        "GPSManager",
                        "Error",
                        "Failed to open GPS log file.",
                        NotificationType::Error
                    );
                }
            }
            else {
                printf("Error: %s exists but is neither serial device nor file.\n", port_or_file);
                notificationManager_.showPopup(
                    "GPSManager",
                    "Error",
                    "Specified path is not a serial device or log file.",
                    NotificationType::Error
                );
                return -1;
            }
        }
        else {
            printf("Error: %s does not exist.\n", port_or_file);
            notificationManager_.showPopup(
                "GPSManager",
                "Error",
                "Specified path does not exist.",
                NotificationType::Error
            );
            return -1;
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


void GPSManager::start() {
    if (gpsThread_.joinable()) return;
    kill_thread_.store(false);
    gpsThread_ = std::thread(&GPSManager::readGPSLoop, this);
}


void GPSManager::stop() {
    kill_thread_.store(true);
    if (gpsThread_.joinable()) {
        gpsThread_.join();
    }
    gps_interface_close(&gps_);
}


void GPSManager::resetSessionData() {
    std::lock_guard<std::mutex> lock(renderLock_);
    memset(&session_, 0, sizeof(full_session_t));
    memset(&cone_session_, 0, sizeof(cone_session_t));
    memset(&user_data_, 0, sizeof(user_data_t));
    user_data_.basepath = user_data_.basepath; 
    user_data_.cone = &cone_;
    user_data_.session = &session_;
    user_data_.cone_session = &cone_session_;
    trajectory_.clear();
    cones_.clear();
    currentPosition_ = ImPlotPoint(0.0, 0.0); 
}


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
            else if (match.message == GPS_UBX_TYPE_NAV_DOP) { 
                std::lock_guard<std::mutex> lock(renderLock_);
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
        }
    }
}


ImPlotPoint GPSManager::getCurrentPosition() const {
    return currentPosition_;
}


std::vector<ImPlotPoint> GPSManager::getTrajectory() const {
    return trajectory_;
}

std::vector<cone_t>& GPSManager::getCones() {
    return cones_;
}


void GPSManager::setConePlacementMode(bool mode) {
    conePlacementMode_ = mode;
}


bool GPSManager::getConePlacementMode() const {
    return conePlacementMode_;
}


gps_serial_port& GPSManager::getGPS() {
    return gps_;
}

std::mutex& GPSManager::getRenderLock() {
    return renderLock_;
}


cone_session_t& GPSManager::getConeSession() {
    return cone_session_;
}


full_session_t& GPSManager::getSession() {
    return session_;
}

void GPSManager::setConeId(cone_id id) {
    cone_.id = id;
}

void GPSManager::clearCones() {
    std::lock_guard<std::mutex> lock(renderLock_);
    cones_.clear();
}

void GPSManager::addCone(const cone_t& cone) {
    std::lock_guard<std::mutex> lock(renderLock_);
    cones_.push_back(cone);
}


int GPSManager::initializeSessions(const std::string& logDir) {
    
    struct stat st = {0};
    if (stat(logDir.c_str(), &st) == -1) {
        if (mkdir(logDir.c_str(), 0700) != 0) {
            printf("Errore nella creazione della directory di log %s\n", logDir.c_str());
            return -1;
        }
    }

    
    if (csv_session_setup(&session_, logDir.c_str()) == -1) {
        printf("Errore: Impostazione della sessione fallita.\n");
        return -1;
    }

    
    
        
        
    

    return 0;
}
