#ifndef GPS_HPP
#define GPS_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include "imgui.h"  

// Forward declarations of structs
struct gps_serial_port;
struct gps_parsed_data_t;
struct cone_t;
struct user_data_t;
struct full_session_t;
struct cone_session_t;

// External variables related to GPS
extern std::mutex renderLock;
extern std::atomic<bool> kill_thread;
extern std::atomic<bool> save_cone;
extern bool conePlacementMode;

extern gps_serial_port gps;
extern gps_parsed_data_t gps_data;
extern cone_t cone;
extern user_data_t user_data;
extern full_session_t session;
extern cone_session_t cone_session;

extern ImVec2 currentPosition;
extern std::vector<ImVec2> trajectory;
extern std::vector<cone_t> cones;

// Declare gpsThread as an external variable
extern std::thread gpsThread;

// Function to handle GPS data in a loop
void readGPSLoop();

// Function to initialize GPS interface
int initializeGPS(const char* port_or_file);

#endif