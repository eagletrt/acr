#pragma once

#include "math.h"
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "utils.hpp"

extern "C" {
#include "utils.h"
}

class Profiler {
public:
  class Stat {
  public:
    std::vector<double> durations;
    std::vector<uint64_t> timestamps_s;
  };

  static bool show;
  static std::map<std::string, Stat> stats;
  static std::mutex mtx;

  static void addMeasure(const std::string &id, double durationSeconds);
  static void updateMetrics(const std::string &filter);
  static void draw();

private:
  static std::vector<std::string> names;
  static std::vector<double> index;
  static std::vector<double> totalTimes;
  static std::vector<double> meanTimes;
  static std::vector<double> meanStd;
  static std::vector<double> cpuTimes;
};

class MeasureScopeTime {
public:
  MeasureScopeTime(const std::string &_id);
  ~MeasureScopeTime();

private:
  std::string id;
  uint64_t start;
};
