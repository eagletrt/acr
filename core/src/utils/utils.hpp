#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <string>

namespace Utils {
std::string getDesktopPath();

bool StringFindCaseInsensitive(const std::string &str,
                               const std::string &search);

enum open_mode {
  open_mode_serial_port,
  open_mode_log_file,
  open_mode_udp,
  open_mode_unknown
};
}; // namespace Utils

#endif
