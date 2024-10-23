#include "utils.hpp"
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

// Function to get the Desktop path
std::string getDesktopPath() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    if (home) {
        return std::string(home) + "\\Desktop";
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Desktop";
    }
#endif
    // Fallback to current directory if HOME is not found
    return ".";
}