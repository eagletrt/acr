#include "gps.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <sys/stat.h>
#include "notifications.hpp"

extern "C" {
    #include "gps.h"
    #include "gps_interface.h"
    #include "acr.h"
    #include "defines.h"
    #include "main.h"
    #include "utils.h"
}



std::mutex renderLock;
std::atomic<bool> kill_thread(false);
std::atomic<bool> save_cone(false);
bool conePlacementMode = false; 

gps_serial_port gps;
gps_parsed_data_t gps_data;
cone_t cone;
user_data_t user_data;
full_session_t session;
cone_session_t cone_session;

ImVec2 currentPosition;
std::vector<ImVec2> trajectory;
std::vector<cone_t> cones;

std::thread gpsThread;

// Function to read GPS data in a separate thread
void readGPSLoop() {
    int fail_count = 0;
    int res = 0;
    unsigned char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
    char line[GPS_MAX_LINE_SIZE];
    while (!kill_thread.load()) {
        int start_size, line_size;
        gps_protocol_type protocol;
        protocol = gps_interface_get_line(&gps, start_sequence, &start_size, line, &line_size, true);
        if (protocol == GPS_PROTOCOL_TYPE_SIZE) {
            fail_count++;
            if (fail_count > 10) {
                printf("Error: GPS disconnected or unable to read data.\n");
                showPopup("GPS_Error", "Error", "GPS disconnected or unable to read data.", NotificationType::Error);
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

        gps_parse_buffer(&gps_data, &match, line, get_t());

        if (match.protocol == GPS_PROTOCOL_TYPE_UBX) {
            if (match.message == GPS_UBX_TYPE_NAV_HPPOSLLH) {
                std::unique_lock<std::mutex> lck(renderLock);
                static double height = 0.0;

                if (CONE_ENABLE_MEAN && currentPosition.x != 0.0 && currentPosition.y != 0.0) {
                    currentPosition.x = currentPosition.x * CONE_MEAN_COMPLEMENTARY + gps_data.hpposllh.lon * (1.0 - CONE_MEAN_COMPLEMENTARY);
                    currentPosition.y = currentPosition.y * CONE_MEAN_COMPLEMENTARY + gps_data.hpposllh.lat * (1.0 - CONE_MEAN_COMPLEMENTARY);
                    height = height * CONE_MEAN_COMPLEMENTARY + gps_data.hpposllh.height * (1.0 - CONE_MEAN_COMPLEMENTARY);
                }
                else {
                    currentPosition.x = gps_data.hpposllh.lon;
                    currentPosition.y = gps_data.hpposllh.lat;
                    height = gps_data.hpposllh.height;
                }

                cone.timestamp = gps_data.hpposllh._timestamp;
                cone.lon = currentPosition.x;
                cone.lat = currentPosition.y;
                cone.alt = height;

                static int count = 0;
                if (session.active && count % 10 == 0) {
                    trajectory.push_back(currentPosition);
                    count = 0;
                }
                count++;
            }
        }

        if (session.active) {
            gps_to_file(&session.files, &gps_data, &match);
        }

        if (save_cone.load()) {
            save_cone.store(false);
            cone_session_write(&cone_session, &cone);
            FILE *tmp = cone_session.file;
            cone_session.file = stdout;
            cone_session_write(&cone_session, &cone);
            cone_session.file = tmp;
            cones.push_back(cone);
        }
    }
}

// Function to initialize GPS interface
int initializeGPS(const char* port_or_file) {
    int res = 0;
    gps_interface_initialize(&gps);
    if (port_or_file) {
        struct stat statbuf;
        if (stat(port_or_file, &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
            // It's a character device (serial port)
            char buff[255];
            snprintf(buff, 255, "sudo chmod 777 %s", port_or_file);
            printf("Changing permissions on serial port: %s with command: %s\n",
                   port_or_file, buff);
            system(buff);
            res = gps_interface_open(&gps, port_or_file, B230400);
        }
        else if (stat(port_or_file, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            // It's a regular file
            printf("Opening file: %s\n", port_or_file);
            res = gps_interface_open_file(&gps, port_or_file);
        }
        else {
            printf("Error: %s does not exist or is not a regular file.\n", port_or_file);
            showPopup("FileBrowser", "Error", "Specified file does not exist.", NotificationType::Error);
        }
        if (res == -1) {
            printf("Error: GPS not found or failed to initialize.\n");
            showPopup("GPS_Error", "Error", "GPS not found or failed to initialize.", NotificationType::Error);
        }
    }
    return res;
}