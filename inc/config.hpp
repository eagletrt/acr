#ifndef CONFIG_HPP
#define CONFIG_HPP

// Directory Paths
#define ASSETS_DIR "../assets/"
#define ICONS_DIR ASSETS_DIR "icons/"
#define FONTS_DIR ASSETS_DIR "fonts/"
#define MAPS_DIR ASSETS_DIR

// GPS Configuration
#ifndef DEFAULT_GPS_PORT
#define DEFAULT_GPS_PORT "/dev/ttyACM0"
#endif

#ifndef DEFAULT_UDP_PORT
#define DEFAULT_UDP_PORT "5005"
#endif

#ifndef GPS_DEFAULT_BAUDRATE
#define GPS_DEFAULT_BAUDRATE B230400
#endif

#endif
