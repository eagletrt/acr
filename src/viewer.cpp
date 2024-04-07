#include "viewer.hpp"

#include <string.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

#include "raylib/src/raylib.h"
extern "C" {
#include "acr.h"
#include "defines.h"
#include "main.h"
#include "utils.h"
}

std::mutex renderLock;
std::atomic<bool> kill_thread;
std::atomic<bool> save_cone;

gps_serial_port gps;
gps_parsed_data_t gps_data;
cone_t cone;
user_data_t user_data;
full_session_t session;
cone_session_t cone_session;

Vector3 latlonheight;
std::vector<Vector3> trajectory;
std::vector<cone_t> cones;
bool calibrated = false;
Vector2 offset;

void readGPSLoop();

#define WIN_W 800
#define WIN_H 800

const double scale = 1000000000.0;
double XX(double x) { return (x - offset.x) * scale; }
double YY(double y) { return (y - offset.y) * scale; }

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Error wrong number of arguments:\n");
    printf("  %s <gps port or log file>\n", argv[0]);
    return -1;
  }
  const char *port_or_file = argv[1];
  const char *basepath = getenv("HOME");

  memset(&session, 0, sizeof(full_session_t));
  memset(&cone_session, 0, sizeof(cone_session_t));
  memset(&user_data, 0, sizeof(user_data_t));
  user_data.basepath = basepath;
  user_data.cone = &cone;
  user_data.session = &session;
  user_data.cone_session = &cone_session;

  int res = 0;
  gps_interface_initialize(&gps);
  if (std::filesystem::is_character_file(port_or_file)) {
    char buff[255];
    snprintf(buff, 255, "sudo chmod 777 %s", port_or_file);
    printf("Changing permissions on serial port: %s with command: %s\n",
           port_or_file, buff);
    system(buff);
    res = gps_interface_open(&gps, port_or_file, B230400);
  } else if (std::filesystem::is_regular_file(port_or_file)) {
    printf("Opening file\n");
    res = gps_interface_open_file(&gps, port_or_file);
  } else {
    printf("%s does not exists\n", port_or_file);
    return -1;
  }
  if (res == -1) {
    printf("GPS not found");
    return -1;
  }

  std::thread gpsThread(readGPSLoop);

  InitWindow(WIN_W, WIN_H, "ACR");
  SetTargetFPS(24);
  Camera2D camera;
  camera.target.x = 0.0;
  camera.target.y = 0.0;
  camera.zoom = 0.02;
  camera.rotation = 0.0;
  camera.offset = {WIN_W / 2.0, WIN_H / 2.0};
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
    if (IsKeyPressed(KEY_O)) {
      cone.id = CONE_ID_ORANGE;
      save_cone.store(true);
    } else if (IsKeyPressed(KEY_Y)) {
      cone.id = CONE_ID_YELLOW;
      save_cone.store(true);
    } else if (IsKeyPressed(KEY_B)) {
      cone.id = CONE_ID_BLUE;
      save_cone.store(true);
    }
    if (save_cone.load() && cone_session.active == 0) {
      if (cone_session_setup(&cone_session, basepath) == -1) {
        printf("Error cone session setup\n");
      }
      if (cone_session_start(&cone_session) == -1) {
        printf("Error cone session start\n");
      }
      printf("Cone session %s started [%s]\n", cone_session.session_name,
             cone_session.session_path);
    }
    if (IsKeyPressed(KEY_C)) {
      camera.target.x = XX(latlonheight.x);
      camera.target.y = YY(latlonheight.y);
    }
    if (IsKeyDown(KEY_UP)) {
      camera.target.y -= 1000;
    }
    if (IsKeyDown(KEY_LEFT)) {
      camera.target.x -= 1000;
    }
    if (IsKeyDown(KEY_DOWN)) {
      camera.target.y += 1000;
    }
    if (IsKeyDown(KEY_RIGHT)) {
      camera.target.x += 1000;
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      const float zoomIncrement = 0.001f;
      camera.zoom += (wheel * zoomIncrement);
      printf("%f\n", camera.zoom);
      if (camera.zoom < zoomIncrement)
        camera.zoom = zoomIncrement;
    }
    if (IsKeyPressed(KEY_S)) {
      printf("%f %f\n", camera.target.x, camera.target.y);
    }

    BeginMode2D(camera);
    for (size_t i = 0; i < trajectory.size(); ++i) {
      DrawCircle(XX(trajectory[i].x), YY(trajectory[i].y), 1000, RAYWHITE);
    }
    for (size_t i = 0; i < cones.size(); ++i) {
      Color c;
      switch (cones[i].id) {
      case CONE_ID_YELLOW:
        c = YELLOW;
        break;
      case CONE_ID_ORANGE:
        c = ORANGE;
        break;
      case CONE_ID_BLUE:
        c = BLUE;
        break;
      default:
        c = RAYWHITE;
        break;
      }
      DrawRectangle(XX(cones[i].lat), YY(cones[i].lon), 1000, 1000, c);
    }
    DrawCircle(XX(latlonheight.x), YY(latlonheight.y), 1000.0, RED);
    EndMode2D();
    EndDrawing();
  }
  kill_thread.store(true);
  gpsThread.join();

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
        printf("New point %f %f\n", gps_data.hpposllh.lat,
               gps_data.hpposllh.lon);

        if (CONE_ENABLE_MEAN && calibrated) {
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

        if (!calibrated && latlonheight.x != 0.0 && latlonheight.y != 0.0) {
          offset.x = latlonheight.x;
          offset.y = latlonheight.y;
          calibrated = true;
        }

        cone.timestamp = gps_data.hpposllh._timestamp;
        cone.lat = latlonheight.x;
        cone.lon = latlonheight.y;
        cone.alt = latlonheight.z;
        if (session.active) {
          trajectory.push_back(latlonheight);
        }
      }
    }

    if (session.active) {
      gps_to_file(&session.files, &gps_data, &match);
    }

    if (save_cone) {
      save_cone.store(false);
      cone_session_write(&cone_session, &cone);
      // print to stdout
      FILE *tmp = cone_session.file;
      cone_session.file = stdout;
      cone_session_write(&cone_session, &cone);
      cone_session.file = tmp;
      cones.push_back(cone);
    }
  }
}
