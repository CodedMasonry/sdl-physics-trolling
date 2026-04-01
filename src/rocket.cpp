#include "rocket.h"
#include "imgui.h"
#include "implot3d.h"
#include <SDL3/SDL.h>

void tick_rocket_simulation(RocketState *rocket_state) {}

void render_rocket_telemetry(RocketState *rocket_state) {
  ImPlot3DAxisFlags flagsXY = ImPlot3DFlags_Equal | ImPlot3DAxisFlags_AutoFit;
  ImPlot3DAxisFlags flagsZ = ImPlot3DAxisFlags_AutoFit;

  ImGui::Begin("Position");
  if (ImPlot3D::BeginPlot("Rocket Position")) {
    ImPlot3D::SetupAxes("Latitude", "Longitude", "Altitude (m)", flagsXY,
                        flagsXY, flagsZ);
    ImPlot3D::EndPlot();
  }
  ImGui::End();
}
