#pragma once
#include "imgui.h"

inline constexpr float GRAVITY = 9.81;

typedef struct RocketState {
  ImVec4 actual_pos = ImVec4(0, 0, 0, 0);
} RocketState;

void tick_rocket_simulation(RocketState *rocket_state);

void render_rocket_telemetry(RocketState *rocket_state);
