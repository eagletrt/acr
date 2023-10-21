#ifndef MAIN_H
#define MAIN_H

#include "gpslib/gps_interface.h"
#include <stdio.h>

typedef enum cone_id {
    CONE_ID_YELLOW = 0,
    CONE_ID_BLUE = 1,
    CONE_ID_ORANGE = 2,

    CONE_ID_SIZE = 3
}cone_id;

typedef enum error_t {
    ERROR_GPIO_INIT,
    ERROR_GPS_NOT_FOUND,
    ERROR_GPS_READ,

    ERORR_SIZE
} error_t;

typedef struct cone_t {
    uint64_t timestamp;
    cone_id id;
    double lat;
    double lon;
    double alt;
}cone_t;

typedef struct full_session_t {
    int active;
    gps_files_t files;
    char session_name[1024];
    char session_path[1024];
}full_session_t;

typedef struct cone_session_t {
    int active;
    FILE *file;
    char session_name[1024];
    char session_path[1024];
}cone_session_t;

typedef struct user_data_t {
    char *basepath;
    int requested_save;

    cone_t *cone;
    full_session_t *session;
    cone_session_t *cone_session;
}user_data_t;

void error_state(error_t error);
const char *error_to_string(error_t error);

void *led_runner();
void sig_handler(int signum);
void pin_setup(user_data_t *user_data);
void pin_interrupt(int gpio, int level, uint32_t tick, void *user_data);

int dir_exist_or_create(char *path);
int dir_next_number(char *path, char *basename);

const char *cone_id_to_string(cone_id id);
int cone_session_setup(cone_session_t *session, char *basepath);
int cone_session_start(cone_session_t *session);
int cone_session_stop(cone_session_t *session);

int csv_session_setup(full_session_t *session, char *basepath);
int csv_session_start(full_session_t *session);
int csv_session_stop(full_session_t *session);

void cone_session_write(cone_session_t *session, cone_t *cone);

#endif // MAIN_H