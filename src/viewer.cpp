#include "viewer.hpp"
#include "raylib/src/raylib.h"
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string.h>
#include <thread>
#include <vector>
extern "C" {
#include "acr.h"
#include "defines.h"
#include "main.h"
#include "utils.h"
}

std::mutex renderLock;
std::atomic<bool> kill_thread;

gps_serial_port gps;
gps_parsed_data_t gps_data;
cone_t cone;
user_data_t user_data;
full_session_t session;
cone_session_t cone_session;

Vector3 latlonheight;
std::vector<Vector3> trajectory;
std::vector<cone_t> cones;

#define WIN_W 800
#define WIN_H 800

int main(void) {
  const char *basepath = getenv("HOME");

  memset(&session, 0, sizeof(full_session_t));
  memset(&cone_session, 0, sizeof(cone_session_t));
  memset(&user_data, 0, sizeof(user_data_t));
  user_data.basepath = basepath;
  user_data.cone = &cone;
  user_data.session = &session;
  user_data.cone_session = &cone_session;

  const char *port = "/dev/ttyACM0";
  if (!std::filesystem::is_character_file(port)) {
    printf("%s does not exists\n", port);
    return -1;
  }
  char buff[255];
  snprintf(buff, 255, "sudo chmod 777 %s", port);
  printf("Changing permissions on serial port: %s with command: %s\n", port,
         buff);
  system(buff);
  gps_interface_initialize(&gps);
  int res = gps_interface_open(&gps, "/dev/ttyACM0", B230400);
  if (res == -1) {
    printf("GPS not found");
    return -1;
  }

  InitWindow(WIN_W, WIN_H, "ACR");
  SetTargetFPS(24);
  Camera2D camera;
  camera.zoom = 1.0;
  camera.target.x = 0.0;
  camera.target.y = 0.0;
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(DARKBROWN);
    DrawText("Quit (ESC)", 10, 10, 20, RAYWHITE);
    DrawText("Trajectory (T)", 10, 30, 20, RAYWHITE);
    DrawText("Cones: ", 10, 50, 20, RAYWHITE);
    DrawText("- Orange (O) ", 20, 70, 20, RAYWHITE);
    DrawText("- Yellow (Y)", 20, 90, 20, RAYWHITE);
    DrawText("- Blue (B)", 20, 110, 20, RAYWHITE);
    DrawText("Center (C)", 10, 130, 20, RAYWHITE);
    DrawText("Move (arrows)", 10, 150, 20, RAYWHITE);

    std::unique_lock<std::mutex> lck(renderLock);
    if (IsKeyPressed(KEY_T)) {
      if (session.active) {
        csv_session_stop(&session);
        printf("Session %s ended\n", session.session_name);
      } else {
        if (csv_session_setup(&session, basepath) == -1) {
          printf("Error session setup\n");
        }
        if (csv_session_start(&session) == -1) {
          printf("Error session start\n");
        }
        printf("Session %s started [%s]\n", session.session_name,
               session.session_path);
      }
    }
    cone.id = CONE_ID_SIZE;
    if (IsKeyPressed(KEY_O)) {
      cone.id = CONE_ID_ORANGE;
    }
    if (IsKeyPressed(KEY_Y)) {
      cone.id = CONE_ID_YELLOW;
    }
    if (IsKeyPressed(KEY_B)) {
      cone.id = CONE_ID_BLUE;
    }
    if (cone.id != CONE_ID_SIZE) {
      if (cone_session.active == 0) {
        if (cone_session_setup(&cone_session, basepath) == -1) {
          printf("Error cone session setup\n");
        }
        if (cone_session_start(&cone_session) == -1) {
          printf("Error cone session start\n");
        }
        printf("Cone session %s started [%s]\n", cone_session.session_name,
               cone_session.session_path);
      }
    }
    if (IsKeyPressed(KEY_C)) {
      camera.target.x = latlonheight.x;
      camera.target.y = latlonheight.y;
    }
    BeginMode2D(camera);
    EndMode2D();
    EndDrawing();
  }

  return 0;
}

void readGPSLoop() {
  int fail_count = 0;
  int res = 0;
  unsigned char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
  char line[GPS_MAX_LINE_SIZE];
  while (!kill_thread) {
    int start_size, line_size;
    gps_protocol_type protocol;
    protocol = gps_interface_get_line(&gps, start_sequence, &start_size, line,
                                      &line_size, true);
    if (protocol == GPS_PROTOCOL_TYPE_SIZE) {
      fail_count++;
      if (fail_count > 10) {
        printf("Error gps disconnected\n");
        return;
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
        std::unique_lock<std::mutex> lck(renderLock);

        if (CONE_ENABLE_MEAN) {
          latlonheight.x =
              latlonheight.x * CONE_MEAN_COMPLEMENTARY +
              gps_data.hpposllh.lat * (1.0 - CONE_MEAN_COMPLEMENTARY);
          latlonheight.y =
              latlonheight.y * CONE_MEAN_COMPLEMENTARY +
              gps_data.hpposllh.lon * (1.0 - CONE_MEAN_COMPLEMENTARY);
          latlonheight.z =
              latlonheight.z * CONE_MEAN_COMPLEMENTARY +
              gps_data.hpposllh.height * (1.0 - CONE_MEAN_COMPLEMENTARY);
        } else {
          latlonheight.x = gps_data.hpposllh.lat;
          latlonheight.y = gps_data.hpposllh.lon;
          latlonheight.z = gps_data.hpposllh.height;
        }

        cone.timestamp = gps_data.hpposllh._timestamp;
        cone.lat = latlonheight.x;
        cone.lon = latlonheight.y;
        cone.alt = latlonheight.z;
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
          // printf("")
        } else {
          // led_blink_once(led_gn, 100);
        }
        request_toggled = 0;
      }
    }
  }
}
