#include "acr.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *error_to_string(acr_error_t error) {
  switch (error) {
    case ERROR_GPIO_INIT:
      return "ERROR_GPIO_INIT";
      break;
    case ERROR_GPS_NOT_FOUND:
      return "ERROR_GPS_NOT_FOUND";
      break;
    case ERROR_GPS_READ:
      return "ERROR_GPS_READ";
      break;
    case ERROR_CONE_SESSION_SETUP:
      return "ERROR_CONE_SESSION_SETUP";
      break;
    case ERROR_CONE_SESSION_START:
      return "ERROR_CONE_SESSION_START";
      break;
    case ERROR_FULL_SESSION_SETUP:
      return "ERROR_FULL_SESSION_SETUP";
      break;
    case ERROR_FULL_SESSION_START:
      return "ERROR_FULL_SESSION_START";
      break;
    default:
      return "ERROR_UNKNOWN";
      break;
  }
}

int dir_exist_or_create(char *path) {
  if (opendir(path) == NULL) {
    if (mkdir(path, 0777) == -1) {
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
  if ((dir = opendir(path)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (ent->d_type == DT_DIR) {
        if (strncmp(ent->d_name, basename, strlen(basename)) == 0) {
          int number = atoi(ent->d_name + strlen(basename));
          if (number > count) {
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

int cone_session_setup(cone_session_t *session, const char *basepath) {
  strcpy(session->session_path, basepath);

  strcat(session->session_path, "/logs/acr/");

  if (dir_exist_or_create(session->session_path) == -1) {
    return -1;
  }

  int session_count = dir_next_number(session->session_path, "cones_");

  snprintf(session->session_name, 1024, "cones_%03d", session_count + 1);
  strcat(session->session_path, session->session_name);

  return 0;
}

int cone_session_start(cone_session_t *session) {
  if (dir_exist_or_create(session->session_path) == -1) {
    return -1;
  }

  char cones_path[2048];
  snprintf(cones_path, 2048, "%s/cones.csv", session->session_path);
  session->file = fopen(cones_path, "w");
  if (session->file == NULL) {
    perror("Could not open cones file");
    return -1;
  }

  fprintf(session->file, "timestamp,cone_id,cone_name,lat,lon,alt\n");
  session->active = 1;

  return 0;
}

int cone_session_stop(cone_session_t *session) {
  fclose(session->file);
  session->active = 0;
  return 0;
}

int csv_session_setup(full_session_t *session, const char *basepath) {
  strcpy(session->session_path, basepath);
  strcat(session->session_path, "/logs/acr/");

  if (dir_exist_or_create(session->session_path) == -1) {
    return -1;
  }

  int session_count = dir_next_number(session->session_path, "trajectory_");

  snprintf(session->session_name, 1024, "trajectory_%03d", session_count + 1);
  strcat(session->session_path, session->session_name);

  return 0;
}
int csv_session_start(full_session_t *session) {
  if (dir_exist_or_create(session->session_path) == -1) {
    return -1;
  }

  char gps_path[2048];
  snprintf(gps_path, 2048, "%s/gps", session->session_path);
  if (dir_exist_or_create(gps_path) == -1) {
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
  fprintf(session->file, "%" PRIu64 ",%d,%s,%f,%f,%f\n", cone->timestamp,
          cone->id, cone_id_to_string(cone->id), cone->lat, cone->lon,
          cone->alt);
  fflush(session->file);
}

const char *cone_id_to_string(cone_id id) {
  switch (id) {
    case CONE_ID_YELLOW:
      return "YELLOW";
      break;
    case CONE_ID_BLUE:
      return "BLUE";
      break;
    case CONE_ID_ORANGE:
      return "ORANGE";
      break;
    default:
      return "UNKNOWN_CONE";
      break;
  }
}
