#include "profiler.hpp"
#include "implot.h"

bool Profiler::show = false;
std::map<std::string, Profiler::Stat> Profiler::stats;
std::mutex Profiler::mtx;

// Statistics
std::vector<std::string> Profiler::names;
std::vector<double> Profiler::index;
std::vector<double> Profiler::totalTimes;
std::vector<double> Profiler::meanTimes;
std::vector<double> Profiler::meanStd;
std::vector<double> Profiler::cpuTimes;

void Profiler::addMeasure(const std::string &id, double durationSeconds) {
  std::unique_lock lck(mtx);
  Stat &stat = stats[id];

  stat.durations.push_back(durationSeconds);
  stat.timestamps_s.push_back(get_t() / 1e6);
}

void Profiler::updateMetrics(const std::string &filter) {
  std::unique_lock lck(mtx);

  names.clear();
  index.clear();
  totalTimes.clear();
  meanTimes.clear();
  meanStd.clear();
  cpuTimes.clear();

  int count = 0;
  for (const auto &[id, stat] : stats) {
    size_t hits = stat.durations.size();
    if (!Utils::StringFindCaseInsensitive(id, filter) || hits == 0) {
      continue;
    }

    names.push_back(id);
    index.push_back(count++);

    double totalDuration = 0;
    for (const auto &d : stat.durations) {
      totalDuration += d;
    }
    totalTimes.push_back(totalDuration);

    double meanDuration = totalDuration / hits;
    meanTimes.push_back(meanDuration);

    double stdDev = 0;
    for (const auto &d : stat.durations) {
      stdDev += (d - meanDuration) * (d - meanDuration) / hits;
    }
    stdDev = sqrt(stdDev / hits);
    meanStd.push_back(stdDev);

    double avgCallsPerSec = hits;
    if (stat.timestamps_s.back() - stat.timestamps_s.front() > 0) {
      avgCallsPerSec /= stat.timestamps_s.back() - stat.timestamps_s.front();
    }
    cpuTimes.push_back(avgCallsPerSec * meanDuration);
  }
}

void Profiler::draw() {
  if (!show) {
    return;
  }
  MeasureScopeTime a("Profiler.Draw");
  static std::string filter;

  // Update data every 30 frames
  static uint64_t frameCounter = 0;
  if (frameCounter++ % 30 == 0) {
    updateMetrics(filter);
  }

  std::unique_lock lck(mtx);
  if (ImGui::Begin("PROFILER", &show)) {
    bool filterFieldFocussed = false;
    ImGui::InputTextWithHint("##profiler_filter", "Filter", &filter);
    if (ImGui::IsItemActive()) {
      filterFieldFocussed = true;
    }
    ImGui::SameLine();
    ImGui::Text("Press R to reset stats");

    if (ImPlot::BeginSubplots("##AxisLinking", 3, 1, ImVec2(-1, -1),
                              ImPlotSubplotFlags_LinkAllX)) {
      if (ImPlot::BeginPlot("Time per second")) {
        if (!index.empty()) {
          ImPlot::PlotBars("##time", &index[0], &cpuTimes[0], index.size(),
                           0.5);
          ImVec4 color = ImPlot::GetLastItemColor();
          for (size_t i = 0; i < index.size(); i++) {
            ImPlot::Annotation(index[i], cpuTimes[i], color, ImVec2(0, 5),
                               false, "%s", names[i].c_str());
            ImPlot::Annotation(index[i], 0, ImVec4(0, 0, 0, 0), ImVec2(0, -1),
                               false, "%.3fs", cpuTimes[i]);
          }
        }
        ImPlot::EndPlot();
      }

      if (ImPlot::BeginPlot("Average Duration")) {
        if (!index.empty()) {
          ImPlot::PlotBars("##mean", &index[0], &meanTimes[0], index.size(),
                           0.5);
          ImVec4 color = ImPlot::GetLastItemColor();
          ImPlot::PlotErrorBars("##std", &index[0], &meanTimes[0], &meanStd[0],
                                index.size());
          for (size_t i = 0; i < index.size(); i++) {
            ImPlot::Annotation(index[i], meanTimes[i], color, ImVec2(0, 5),
                               false, "%s", names[i].c_str());
            ImPlot::Annotation(index[i], 0, ImVec4(0, 0, 0, 0), ImVec2(0, -1),
                               false, "%.3fs", meanTimes[i]);
          }
        }
        ImPlot::EndPlot();
      }

      if (ImPlot::BeginPlot("Total Duration")) {
        if (!index.empty()) {
          ImPlot::PlotBars("##total", &index[0], &totalTimes[0], index.size(),
                           0.5);
          ImVec4 color = ImPlot::GetLastItemColor();
          for (size_t i = 0; i < index.size(); i++) {
            ImPlot::Annotation(index[i], totalTimes[i], color, ImVec2(0, 5),
                               false, "%s", names[i].c_str());
            ImPlot::Annotation(index[i], 0, ImVec4(0, 0, 0, 0), ImVec2(0, -1),
                               false, "%.3fs", totalTimes[i]);
          }
        }
        ImPlot::EndPlot();
      }

      ImPlot::EndSubplots();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
      stats.clear();
      printf("Profiler, reset timestamps");
    }
    ImGui::End();
  }
}

MeasureScopeTime::MeasureScopeTime(const std::string &_id) : id(_id) {
  start = get_t();
}

MeasureScopeTime::~MeasureScopeTime() {
  Profiler::addMeasure(id, (get_t() - start) / 1e6);
}
