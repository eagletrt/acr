#define STB_IMAGE_IMPLEMENTATION
#include "viewer.hpp"

#include <GLFW/glfw3.h>
#include <string.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "stb_image.h"

extern "C"
{
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

ImVec2 lonlat;
std::vector<ImVec2> trajectory;
std::vector<cone_t> cones;

ImVec2 povoBoundTL{11.1484815430000008, 46.0658863580000002};
ImVec2 povoBoundBR{11.1515535430000003, 46.0689583580000033};
ImVec2 vadenaBoundTL{11.3097566090000008, 46.4300119620000018};
ImVec2 vadenaBoundBR{11.3169246090000009, 46.4382039620000029};
void readGPSLoop();

#define WIN_W 800
#define WIN_H 800

ImTextureID loadImagePNG(const char *path);
GLFWwindow *setupImGui();
void startFrame();
void endFrame(GLFWwindow *window);

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Error wrong number of arguments:\n");
    printf("  %s <gps port or log file>\n", argv[0]);
    return -1;
  }
  const char *port_or_file = argv[1];
  const char *basepath = getenv("HOME");

  GLFWwindow *window = setupImGui();
  if (window == nullptr)
  {
    return -1;
  }

  memset(&session, 0, sizeof(full_session_t));
  memset(&cone_session, 0, sizeof(cone_session_t));
  memset(&user_data, 0, sizeof(user_data_t));
  user_data.basepath = basepath;
  user_data.cone = &cone;
  user_data.session = &session;
  user_data.cone_session = &cone_session;

  int res = 0;
  gps_interface_initialize(&gps);
  if (std::filesystem::is_character_file(port_or_file))
  {
    char buff[255];
    snprintf(buff, 255, "sudo chmod 777 %s", port_or_file);
    printf("Changing permissions on serial port: %s with command: %s\n",
           port_or_file, buff);
    system(buff);
    res = gps_interface_open(&gps, port_or_file, B230400);
  }
  else if (std::filesystem::is_regular_file(port_or_file))
  {
    printf("Opening file\n");
    res = gps_interface_open_file(&gps, port_or_file);
  }
  else
  {
    printf("%s does not exists\n", port_or_file);
    return -1;
  }
  if (res == -1)
  {
    printf("GPS not found");
    return -1;
  }

  ImTextureID povoTex = loadImagePNG("assets/povo.png");
  ImTextureID vadenaTex = loadImagePNG("assets/vadena.png");
  std::thread gpsThread(readGPSLoop);

  int mapIndex = 0;
  float mapOpacity = 0.5f;
  while (!glfwWindowShouldClose(window))
  {
    startFrame();

    ImGui::Begin("ACR");

    if (ImGui::TreeNode("Help"))
    {
      ImGui::Text("Quit (Q)");
      ImGui::Text("Trajectory (T)");
      ImGui::Text("Cones: ");
      ImGui::Text("- Orange (O)");
      ImGui::Text("- Yellow (Y)");
      ImGui::Text("- Blue (B)");
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Settings"))
    {
      ImGui::SliderFloat("Map Opacity", &mapOpacity, 0.0f, 1.0f);
      ImGui::RadioButton("Povo", &mapIndex, 0);
      ImGui::RadioButton("Vadena", &mapIndex, 1);
      ImGui::TreePop();
    }

    std::unique_lock<std::mutex> lck(renderLock);
    if (ImGui::IsKeyPressed(ImGuiKey_T))
    {
      if (session.active)
      {
        csv_session_stop(&session);
        printf("Session %s ended\n", session.session_name);
      }
      else
      {
        if (csv_session_setup(&session, basepath) == -1)
        {
          printf("Error session setup\n");
        }
        if (csv_session_start(&session) == -1)
        {
          printf("Error session start\n");
        }
        printf("Session %s started [%s]\n", session.session_name,
               session.session_path);
      }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_O))
    {
      cone.id = CONE_ID_ORANGE;
      save_cone.store(true);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Y))
    {
      cone.id = CONE_ID_YELLOW;
      save_cone.store(true);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_B))
    {
      cone.id = CONE_ID_BLUE;
      save_cone.store(true);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Q))
    {
      glfwSetWindowShouldClose(window, true);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_C))
    {
      trajectory.clear();
      cones.clear();
    }
    if (save_cone.load() && cone_session.active == 0)
    {
      if (cone_session_setup(&cone_session, basepath) == -1)
      {
        printf("Error cone session setup\n");
      }
      if (cone_session_start(&cone_session) == -1)
      {
        printf("Error cone session start\n");
      }
      printf("Cone session %s started [%s]\n", cone_session.session_name,
             cone_session.session_path);
    }

    ImVec2 size = ImGui::GetContentRegionAvail();
    if (ImPlot::BeginPlot("GpsPositions", size, ImPlotFlags_Equal))
    {
      if (mapIndex == 0)
      {
        ImPlot::PlotImage("Povo", povoTex, povoBoundTL, povoBoundBR,
                          ImVec2(0, 0), ImVec2(1, 1),
                          ImVec4(1, 1, 1, mapOpacity));
      }
      else
      {
        ImPlot::PlotImage("Vadena", vadenaTex, vadenaBoundTL, vadenaBoundBR,
                          ImVec2(0, 0), ImVec2(1, 1),
                          ImVec4(1, 1, 1, mapOpacity));
      }

      ImPlot::PlotScatter("Trajectory", &trajectory[0].x, &trajectory[0].y,
                          trajectory.size(), 0, 0, sizeof(ImVec2));

      for (size_t i = 0; i < cones.size(); ++i)
      {
        ImVec4 c;
        switch (cones[i].id)
        {
        case CONE_ID_YELLOW:
          c = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
          break;
        case CONE_ID_ORANGE:
          c = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
          break;
        case CONE_ID_BLUE:
          c = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
          break;
        default:
          c = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
          break;
        }
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Up, 8, c, 0.0);
        ImPlot::PlotScatter("Cones", &cones[i].lon, &cones[i].lat, 1);
      }
      ImPlot::PlotScatter("Current", &lonlat.x, &lonlat.y, 1);
      ImPlot::EndPlot();
    }
    ImGui::End();

    endFrame(window);
  }
  kill_thread.store(true);
  gpsThread.join();

  return 0;
}

void readGPSLoop()
{
  int fail_count = 0;
  int res = 0;
  unsigned char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
  char line[GPS_MAX_LINE_SIZE];
  while (!kill_thread)
  {
    int start_size, line_size;
    gps_protocol_type protocol;
    protocol = gps_interface_get_line(&gps, start_sequence, &start_size, line,
                                      &line_size, true);
    if (protocol == GPS_PROTOCOL_TYPE_SIZE)
    {
      fail_count++;
      if (fail_count > 10)
      {
        printf("Error gps disconnected\n");
        return;
      }
      continue;
    }
    else
    {
      fail_count = 0;
    }

    gps_protocol_and_message match;
    res = gps_match_message(&match, line, protocol);
    if (res == -1)
    {
      continue;
    }

    gps_parse_buffer(&gps_data, &match, line, get_t());

    if (match.protocol == GPS_PROTOCOL_TYPE_UBX)
    {
      if (match.message == GPS_UBX_TYPE_NAV_HPPOSLLH)
      {
        std::unique_lock<std::mutex> lck(renderLock);
        static double height = 0.0;

        if (CONE_ENABLE_MEAN && lonlat.x != 0.0 && lonlat.y != 0.0)
        {
          lonlat.x = lonlat.x * CONE_MEAN_COMPLEMENTARY +
                     gps_data.hpposllh.lon * (1.0 - CONE_MEAN_COMPLEMENTARY);
          lonlat.y = lonlat.y * CONE_MEAN_COMPLEMENTARY +
                     gps_data.hpposllh.lat * (1.0 - CONE_MEAN_COMPLEMENTARY);
          height = height * CONE_MEAN_COMPLEMENTARY +
                   gps_data.hpposllh.height * (1.0 - CONE_MEAN_COMPLEMENTARY);
        }
        else
        {
          lonlat.x = gps_data.hpposllh.lon;
          lonlat.y = gps_data.hpposllh.lat;
          height = gps_data.hpposllh.height;
        }

        cone.timestamp = gps_data.hpposllh._timestamp;
        cone.lon = lonlat.x;
        cone.lat = lonlat.y;
        cone.alt = height;

        static int count = 0;
        if (session.active && count % 10 == 0)
        {
          trajectory.push_back(lonlat);
          count = 0;
        }
        count++;
      }
    }

    if (session.active)
    {
      gps_to_file(&session.files, &gps_data, &match);
    }

    if (save_cone)
    {
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

ImTextureID loadImagePNG(const char *path)
{
  int width, height;
  unsigned char *data = stbi_load(path, &width, &height, 0, 4);
  if (data == NULL)
  {
    printf("Error loading image: %s\n", path);
    return 0;
  }
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);
  return (ImTextureID)(intptr_t)tex;
}

GLFWwindow *setupImGui()
{
  if (!glfwInit())
    return nullptr;

  // Create window with graphics context
  GLFWwindow *window = glfwCreateWindow(1280, 720, "ACR", nullptr, nullptr);
  if (window == nullptr)
    return nullptr;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL2_Init();
  return window;
}
void startFrame()
{
  glfwPollEvents();

  // Start the Dear ImGui frame
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}
void endFrame(GLFWwindow *window)
{
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
               clear_color.z * clear_color.w, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  // If you are using this code with non-legacy OpenGL header/contexts (which
  // you should not, prefer using imgui_impl_opengl3.cpp!!), you may need to
  // backup/reset/restore other state, e.g. for current shader using the
  // commented lines below.
  // GLint last_program;
  // glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
  // glUseProgram(0);
  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
  // glUseProgram(last_program);

  glfwMakeContextCurrent(window);
  glfwSwapBuffers(window);
}