#include "utils.hpp"
#include "config.hpp"
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

// Function to get the Desktop path
std::string Utils::getDesktopPath() {
#ifdef _WIN32
  CHAR path[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
    return std::string(path);
  }
#else
  const char *home = getenv("HOME");
  if (home) {
    return std::string(home) + "/Desktop";
  }
#endif
  // Fallback to current directory if HOME is not found
  return ".";
}

bool Utils::StringFindCaseInsensitive(const std::string &str,
                                      const std::string &search) {
  auto iter = std::search(str.begin(), str.end(), search.begin(), search.end(),
                          [](auto ch1, auto ch2) {
                            return std::tolower(ch1) == std::tolower(ch2);
                          });

  return (iter != str.end());
}
