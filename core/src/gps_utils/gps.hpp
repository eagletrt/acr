
#ifndef GPS_HPP
#define GPS_HPP

#include "config.hpp"
#include "imgui.h"
#include "implot.h"
#include "notifications.hpp"
#include "utils.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

extern "C" {
#include "/usr/include/gps.h"
#include "acr.h"
#include "defines.h"
#include "gps_interface.h"
#include "main.h"
#include "utils.h"
}

class NotificationManager;

class GPSManager {
public:
  GPSManager(NotificationManager &notificationManager);
  ~GPSManager();

  int initialize(const char *port_or_file);

  void start();
  void stop();

  void resetSessionData();

  ImPlotPoint getCurrentPosition() const;
  std::vector<ImPlotPoint> getTrajectory() const;
  std::vector<cone_t> &getCones();

  void setConePlacementMode(bool mode);
  bool getConePlacementMode() const;

  gps_serial_port &getGPS();
  cone_session_t &getConeSession();
  full_session_t &getSession();

  std::mutex &getRenderLock();

  gps_parsed_data_t getGPSData() const;

  float getHDOP();
  float getPDOP();

  std::pair<float, uint64_t> getPVT();

  std::atomic<bool> saveCone_;

  void deleteCone(int index);
  void setConeId(cone_id id);
  void clearCones();
  void addCone(const cone_t &cone);
  mutable std::mutex renderLock_;
  int initializeSessions(const std::string &logDir);

  void setOpenMode(Utils::open_mode mode);

private:
  std::atomic<bool> kill_thread_;
  bool conePlacementMode_;

  gps_serial_port gps_;
  gps_parsed_data_t gps_data_;
  struct gps_data_t gpsd_data_;
  cone_t cone_;
  user_data_t user_data_;
  Utils::open_mode open_mode;
  full_session_t session_;
  cone_session_t cone_session_;

  ImPlotPoint currentPosition_;
  std::vector<ImPlotPoint> trajectory_;
  std::vector<cone_t> cones_;

  std::thread gpsThread_;
  NotificationManager &notificationManager_;

  void readGPSLoop();
  void readGPSDLoop();
};

#endif
