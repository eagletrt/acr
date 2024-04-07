#ifndef ACR_H
#define ACR_H

#include "gpslib/gps_interface.h"
#include <stdint.h>

typedef enum cone_id {
  CONE_ID_YELLOW = 0,
  CONE_ID_BLUE = 1,
  CONE_ID_ORANGE = 2,

  CONE_ID_SIZE = 3
} cone_id;

typedef enum acr_error_t {
  ERROR_GPIO_INIT,
  ERROR_GPS_NOT_FOUND,
  ERROR_GPS_READ,
  ERROR_CONE_SESSION_SETUP,
  ERROR_CONE_SESSION_START,
  ERROR_FULL_SESSION_SETUP,
  ERROR_FULL_SESSION_START,

  ERORR_SIZE
} acr_error_t;

typedef struct cone_t {
  uint64_t timestamp;
  cone_id id;
  double lat;
  double lon;
  double alt;
} cone_t;

typedef struct full_session_t {
  int active;
  gps_files_t files;
  char session_name[1024];
  char session_path[1024];
} full_session_t;

typedef struct cone_session_t {
  int active;
  FILE *file;
  char session_name[1024];
  char session_path[1024];
} cone_session_t;

typedef struct user_data_t {
  const char *basepath;
  int requested_save;

  cone_t *cone;
  full_session_t *session;
  cone_session_t *cone_session;
} user_data_t;

const char *error_to_string(acr_error_t error);

int dir_exist_or_create(char *path);
int dir_next_number(char *path, char *basename);

const char *cone_id_to_string(cone_id id);
int cone_session_setup(cone_session_t *session, const char *basepath);
int cone_session_start(cone_session_t *session);
int cone_session_stop(cone_session_t *session);

int csv_session_setup(full_session_t *session, const char *basepath);
int csv_session_start(full_session_t *session);
int csv_session_stop(full_session_t *session);

void cone_session_write(cone_session_t *session, cone_t *cone);

#endif // ACR_H
