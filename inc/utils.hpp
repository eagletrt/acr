#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace Utils {
std::string getDesktopPath();
enum open_mode {
  open_mode_serial_port,
  open_mode_log_file,
  open_mode_udp,
  open_mode_unknown
};
}; // namespace Utils

#endif
