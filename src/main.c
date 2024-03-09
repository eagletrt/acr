#include "main.h"

#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "defines.h"
#include "gpio.h"
#include "led.h"
#include "utils.h"

pthread_t led_thread;
int in_error_state = 0;
int kill_thread = 0;
led_t *led_gn;
led_t *led_rd;

int main(void) {
  printf("ACR: Advanced Cone Registration\n");
  user_data_t user_data;
  led_gn = led_new(P_LED_GN);
  led_rd = led_new(P_LED_RD);

  cone_t cone;
  full_session_t session;
  cone_session_t cone_session;
  memset(&session, 0, sizeof(full_session_t));
  memset(&cone_session, 0, sizeof(cone_session_t));
  memset(&user_data, 0, sizeof(user_data_t));

  // char *basepath = getenv("USER");
  char *basepath = "/home/philpi";

  if (gpioInitialise() == PI_INIT_FAILED) {
    error_state(ERROR_GPIO_INIT);
  }

  signal(SIGINT, sig_handler);
  signal(SIGKILL, sig_handler);

  user_data.basepath = basepath;
  user_data.cone = &cone;
  user_data.session = &session;
  user_data.cone_session = &cone_session;

  pin_setup(&user_data);

  pthread_create(&led_thread, NULL, led_runner, NULL);

  led_set_state(led_gn, 200, 300);
  led_set_state(led_rd, 200, 300);

  gps_serial_port gps;
  gps_interface_initialize(&gps);
  int res = gps_interface_open(&gps, "/dev/ttyACM0", B230400);
  if (res == -1) {
    error_state(ERROR_GPS_NOT_FOUND);
  }

  unsigned char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
  char line[GPS_MAX_LINE_SIZE];

  double lat, lon, alt;
  gps_parsed_data_t gps_data;

  usleep(1e6);

  led_set_state(led_gn, 0, 0);
  led_set_state(led_rd, 0, 0);

  int fail_count = 0;
  while (!kill_thread) {
    int start_size, line_size;
    gps_protocol_type protocol;
    protocol = gps_interface_get_line(&gps, start_sequence, &start_size, line,
                                      &line_size, true);
    if (protocol == GPS_PROTOCOL_TYPE_SIZE) {
      fail_count++;
      if (fail_count > 10) {
        error_state(ERROR_GPS_READ);
      }
      continue;
    } else {
      fail_count = 0;
    }

    gps_protocol_and_message match;
    res = gps_match_message(&match, line, protocol);
    if (res == -1) {
      continue;
    }

    gps_parse_buffer(&gps_data, &match, line, get_t());

    if (match.protocol == GPS_PROTOCOL_TYPE_UBX) {
      if (match.message == GPS_UBX_TYPE_NAV_HPPOSLLH) {
        lat = gps_data.hpposllh.lat;
        lon = gps_data.hpposllh.lon;
        alt = gps_data.hpposllh.height;

        cone.timestamp = gps_data.hpposllh._timestamp;
        if (CONE_ENABLE_MEAN && user_data.requested_save) {
          cone.lat = cone.lat * CONE_MEAN_COMPLEMENTARY +
                     lat * (1.0 - CONE_MEAN_COMPLEMENTARY);
          cone.lon = cone.lon * CONE_MEAN_COMPLEMENTARY +
                     lon * (1.0 - CONE_MEAN_COMPLEMENTARY);
          cone.alt = cone.alt * CONE_MEAN_COMPLEMENTARY +
                     alt * (1.0 - CONE_MEAN_COMPLEMENTARY);
        } else {
          cone.lat = lat;
          cone.lon = lon;
          cone.alt = alt;
        }
      }
    }

    if (session.active) {
      gps_to_file(&session.files, &gps_data, &match);
    }

    static uint64_t cone_t = 0;
    static int request_toggled = 0;
    if (user_data.requested_save && request_toggled == 0) {
      user_data.requested_save = 0;
      request_toggled = 1;
      cone_t = get_t();
    }
    if (request_toggled) {
      if (CONE_ENABLE_MEAN == 0 || get_t() - cone_t > CONE_REPRESS_US) {
        cone_session_write(&cone_session, &cone);
        // print to stdout
        FILE *tmp = cone_session.file;
        cone_session.file = stdout;
        cone_session_write(&cone_session, &cone);
        cone_session.file = tmp;

        if (CONE_ENABLE_MEAN) {
          led_off(led_gn);
        } else {
          led_blink_once(led_gn, 100);
        }
        request_toggled = 0;
      }
    }
  }

  return EXIT_SUCCESS;
}

const char *error_to_string(error_t error) {
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

void error_state(error_t err) {
  in_error_state = 1;
  led_off(led_gn);
  led_off(led_rd);

  int total_blink_ms = 2000;
  int increment_ms = total_blink_ms / (ERORR_SIZE * 2);
  printf("\r\nError: %s\n", error_to_string(err));
  while (true) {
    for (uint8_t i = 0; i <= err; i++) {
      led_blink_once(led_rd, increment_ms);
      usleep(2 * increment_ms * 1e3);
    }
    usleep((total_blink_ms - increment_ms) * 1e3);
  }
}

void *led_runner() {
  while (!kill_thread) {
    led_run();
    usleep(1000);
  }
  return NULL;
}

void sig_handler(int signum) {
  if (signum == SIGKILL || signum == SIGINT) {
    kill_thread = 1;
    pthread_join(led_thread, NULL);
    gpioWrite(P_LED_GN, 0);
    gpioWrite(P_LED_RD, 0);
    gpioTerminate();
    printf("\rExiting\n");
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
  (void)tick;
  if (gpioSkipForDebounce(gpio, level))
    return;
  if (level != 0)
    return;

  if (in_error_state) {
    if (!gpioRead(P_BTN_BL) && !gpioRead(P_BTN_OR)) {
      printf("\nRequested kill\n");
      raise(SIGKILL);
    }
    return;
  }

  user_data_t *data = (user_data_t *)user_data;

  switch (gpio) {
  case P_BTN_MODE:
    if (data->session->active) {
      csv_session_stop(data->session);
      printf("Session %s ended\n", data->session->session_name);
      led_off(led_rd);
    } else {
      if (csv_session_setup(data->session, data->basepath) == -1) {
        error_state(ERROR_FULL_SESSION_SETUP);
      }
      if (csv_session_start(data->session) == -1) {
        error_state(ERROR_FULL_SESSION_START);
      }
      printf("Session %s started [%s]\n", data->session->session_name,
             data->session->session_path);
      led_on(led_rd);
    }
    break;
  case P_BTN_YL:
  case P_BTN_OR:
  case P_BTN_BL:
    if (data->cone_session->active == 0) {
      if (cone_session_setup(data->cone_session, data->basepath) == -1) {
        error_state(ERROR_CONE_SESSION_SETUP);
      }
      if (cone_session_start(data->cone_session) == -1) {
        error_state(ERROR_CONE_SESSION_START);
      }
      printf("Cone session %s started [%s]\n", data->cone_session->session_name,
             data->cone_session->session_path);
    }
    break;
  default:
    break;
  }

  static uint64_t t_cone[CONE_ID_SIZE];
  if (gpio == P_BTN_YL || gpio == P_BTN_BL || gpio == P_BTN_OR) {
    if (gpio == P_BTN_YL) {
      data->cone->id = CONE_ID_YELLOW;
    } else if (gpio == P_BTN_BL) {
      data->cone->id = CONE_ID_BLUE;
    } else if (gpio == P_BTN_OR) {
      data->cone->id = CONE_ID_ORANGE;
    }

    if (get_t() - t_cone[data->cone->id] > CONE_REPRESS_US) {
      data->requested_save = 1;
      led_on(led_gn);
    }

    // Update time for each cone.
    // Preventing that two cones are saved simultaneously
    for (int i = 0; i < CONE_ID_SIZE; i++) {
      t_cone[i] = get_t();
    }
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

int cone_session_setup(cone_session_t *session, char *basepath) {
  strcpy(session->session_path, basepath);

  strcat(session->session_path, "/logs/acr/");

  if (dir_exist_or_create(session->session_path) == -1) {
    return -1;
  }

  int session_count = dir_next_number(session->session_path, "cones_");

  snprintf(session->session_name, 1025, "cones_%03d", session_count + 1);
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

int csv_session_setup(full_session_t *session, char *basepath) {
  strcpy(session->session_path, basepath);
  strcat(session->session_path, "/logs/acr/");

  if (dir_exist_or_create(session->session_path) == -1) {
    return -1;
  }

  int session_count = dir_next_number(session->session_path, "trajectory_");

  snprintf(session->session_name, 1025, "trajectory_%03d", session_count + 1);
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
