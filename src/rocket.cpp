#include "rocket.h"
#include "imgui.h"
#include <SDL3/SDL.h>

void tick_rocket_simulation(RocketState *rocket_state) {}

void render_rocket_telemetry(RocketState *rocket_state) {
  ImGui::Begin("Realtime Plot");
  ImGui::Text("Move your mouse to change the data!");
  ImGui::End();
}
