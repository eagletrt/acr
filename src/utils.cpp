#include "utils.hpp"
#include <cstdlib>

#ifdef _WIN32
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
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Desktop";
    }
#endif
    // Fallback to current directory if HOME is not found
    return ".";
}
