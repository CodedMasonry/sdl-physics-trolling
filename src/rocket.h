#pragma once
#include "imgui.h"

typedef struct RocketState {
  ImVec4 actual_pos = ImVec4(0, 0, 0, 0);
} RocketState;

void render_rocket_telemetry();
