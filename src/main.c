#include "main.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include "defines.h"
#include "gpio.h"
#include "utils.h"
#include "led.h"

led_t *led_gn;
led_t *led_rd;

int main(void) {
    printf("ACR: Advanced Cone Registration\n");
    user_data_t user_data;

    cone_t cone;
    full_session_t session;
    cone_session_t cone_session;
    memset(&session, 0, sizeof(full_session_t));
    memset(&cone_session, 0, sizeof(cone_session_t));

    // char *basepath = getenv("USER");
    char *basepath = "/home/philpi";

    if(gpioInitialise() == PI_INIT_FAILED) {
	    printf("Error initializing pigpio\n");
	    exit(EXIT_FAILURE);
    }

	signal(SIGINT, sig_handler);
	signal(SIGKILL, sig_handler);

    user_data.basepath = basepath;
    user_data.cone = &cone;
    user_data.session = &session;
    user_data.cone_session = &cone_session;

    pin_setup(&user_data);

    led_gn = led_new(P_LED_GN);
    led_rd = led_new(P_LED_RD);

    led_set_state(led_gn, 100, 100);
    led_set_state(led_rd, 1000, 500);

    gps_serial_port gps;
    int res = gps_interface_open(&gps, "/dev/ttyACM0", B230400);
    // int res = gps_interface_open_file(&gps, "/home/philpi/gps_0.log");
    if(res == -1) {
	    printf("Could not open gps serial port\n");
	    exit(EXIT_FAILURE);
    }

    const char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
    char line[GPS_MAX_LINE_SIZE];

    double lat, lon, alt;
    gps_parsed_data_t gps_data;

    while(1) {
	    led_run();

	    int start_size, line_size;
        gps_protocol_type protocol;
	    protocol = gps_interface_get_line(&gps, start_sequence, &start_size, line, &line_size, true);
        if(protocol == GPS_PROTOCOL_TYPE_SIZE) {
            continue;
        }

        gps_protocol_and_message match;
        res = gps_match_message(&match, line, protocol);
        if(res == -1) {
            continue;
        }

        gps_parse_buffer(&gps_data, &match, line, get_t());

        if(match.protocol == GPS_PROTOCOL_TYPE_UBX) {
            if(match.message == GPS_UBX_TYPE_NAV_HPPOSLLH) {
                lat = gps_data.hpposllh.lat;
                lon = gps_data.hpposllh.lon;
                alt = gps_data.hpposllh.height;

                cone.timestamp = gps_data.hpposllh._timestamp;
                cone.lat = lat;
                cone.lon = lon;
                cone.alt = alt;
            }
        }

        if(session.active) {
            gps_to_file(&session.files, &gps_data, &match);
        }
    }

    return EXIT_SUCCESS;
}

void sig_handler(int signum) {
	if(signum == SIGKILL || signum == SIGINT) {
		gpioWrite(P_LED_GN, 0);
		gpioWrite(P_LED_RD, 0);
		gpioTerminate();
		exit(EXIT_FAILURE);
	}
}

void pin_setup(user_data_t *user_data) {
    gpioSetMode(P_LED_GN, PI_OUTPUT);
    gpioSetMode(P_LED_RD, PI_OUTPUT);
    gpioSetMode(P_BTN_YL, PI_INPUT);
    gpioSetMode(P_BTN_OR, PI_INPUT);
    gpioSetMode(P_BTN_BL, PI_INPUT);
    gpioSetMode(P_BTN_MODE, PI_INPUT);

    gpioSetPullUpDown(P_BTN_YL, PI_PUD_UP);
    gpioSetPullUpDown(P_BTN_OR, PI_PUD_UP);
    gpioSetPullUpDown(P_BTN_BL, PI_PUD_UP);
    gpioSetPullUpDown(P_BTN_MODE, PI_PUD_UP);

    gpioSetAlertFuncEx(P_BTN_YL, pin_interrupt, user_data);
    gpioSetAlertFuncEx(P_BTN_OR, pin_interrupt, user_data);
    gpioSetAlertFuncEx(P_BTN_BL, pin_interrupt, user_data);
    gpioSetAlertFuncEx(P_BTN_MODE, pin_interrupt, user_data);
}

void pin_interrupt(int gpio, int level, uint32_t tick, void *user_data) {
    if(gpioSkipForDebounce(gpio, level)) return;
    if(level != 0) return;

    user_data_t *data = (user_data_t *)user_data;

    switch(gpio) {
        case P_BTN_MODE:
            if(data->session->active) {
                csv_session_stop(data->session);
                printf("Session ended\n");
                led_set_state(led_rd, 0, 0);
            } else {
                csv_session_setup(data->session, data->basepath);
                csv_session_start(data->session);
                printf("Session started\n");
                led_set_state(led_rd, 1000, 0);
            }
        break;
        case P_BTN_YL:
        case P_BTN_OR:
        case P_BTN_BL:
            if(data->cone_session->active == 0) {
                cone_session_setup(data->cone_session, data->basepath);
                cone_session_start(data->cone_session);
                printf("Cone session started\n");
            }
        break;
        default:
            break;
    }

    static uint64_t t_cone[CONE_ID_SIZE];
    if(gpio == P_BTN_YL || gpio == P_BTN_BL || gpio == P_BTN_OR) {
        if(gpio == P_BTN_YL) {
            data->cone->id = CONE_ID_YELLOW;
        } else if(gpio == P_BTN_BL) {
            data->cone->id = CONE_ID_BLUE;
        } else if(gpio == P_BTN_OR) {
            data->cone->id = CONE_ID_ORANGE;
        }

        if(get_t() - t_cone[data->cone->id] > CONE_REPRESS_US) {
            cone_session_write(data->cone_session, data->cone);
            led_blink_once(led_gn, 100);
        }
        t_cone[data->cone->id] = get_t();
    }
}

int dir_exist_or_create(char *path) {
    DIR *dir;
    struct dirent *ent;
    if(opendir(path) == NULL) {
        if(mkdir(path, 0777) == -1) {
            fprintf(stderr, "Could not create directory %s\n", path);
            return -1;
        }
    }
    return 0;
}

int dir_next_number(char *path, char *basename) {
    DIR *dir;
    int count = 0;
    struct dirent *ent;
    if((dir = opendir(path)) != NULL) {
        while((ent = readdir(dir)) != NULL) {
            if(ent->d_type == DT_DIR) {
                if(strncmp(ent->d_name, basename, strlen(basename)) == 0) {
                    int number = atoi(ent->d_name + strlen(basename));
                    if(number > count) {
                        count = number;
                    }
                }
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "Could not open directory %s\n", path);
        return -1;
    }
    return count;
}

int cone_session_setup(cone_session_t *session, char *basepath) {
    strcpy(session->session_path, basepath);

    strcat(session->session_path, "/logs/acr/");

    if(dir_exist_or_create(session->session_path) == -1) {
        return -1;
    }
    
    int session_count = dir_next_number(session->session_path, "cones_");

    snprintf(session->session_name, 1025, "cones_%03d", session_count + 1);
    strcat(session->session_path, session->session_name);

    return 0;
}

int cone_session_start(cone_session_t *session) {
    if(dir_exist_or_create(session->session_path) == -1) {
        return -1;
    }

    char cones_path[2048];
    snprintf(cones_path, 2048, "%s/cones.csv", session->session_path);
    session->file = fopen(cones_path, "w");
    if(session->file == NULL) {
        perror("Could not open cones file");
        return -1;
    }

    fprintf(session->file, "timestamp,id,lat,lon,alt\n");
    session->active = 1;

    return 0;
}

int cone_session_stop(cone_session_t *session) {
    fclose(session->file);
    session->active = 0;
    return 0;
}

int csv_session_setup(full_session_t *session, char *basepath) {
    strcpy(session->session_path, basepath);
    strcat(session->session_path, "/logs/acr/");

    if(dir_exist_or_create(session->session_path) == -1) {
        return -1;
    }
    
    int session_count = dir_next_number(session->session_path, "trajectory_");

    snprintf(session->session_name, 1025, "trajectory_%03d", session_count + 1);
    strcat(session->session_path, session->session_name);

    return 0;
}
int csv_session_start(full_session_t *session) {
    if(dir_exist_or_create(session->session_path) == -1) {
        return -1;
    }

    char gps_path[2048];
    snprintf(gps_path, 2048, "%s/gps", session->session_path);
    if(dir_exist_or_create(gps_path) == -1) {
        return -1;
    }

    gps_open_files(&session->files, gps_path);
    gps_header_to_file(&session->files);

    session->active = 1;

    return 0;
}
int csv_session_stop(full_session_t *session) {
    gps_close_files(&session->files);
    session->active = 0;
    return 0;
}

void cone_session_write(cone_session_t *session, cone_t *cone) {
    fprintf(session->file, "%" PRIu64 ",%d,%f,%f,%f\n", cone->timestamp, cone->id, cone->lat, cone->lon, cone->alt);
    fflush(session->file);
}
