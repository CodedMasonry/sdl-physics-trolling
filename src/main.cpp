#define SDL_MAIN_USE_CALLBACKS
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "implot3d.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

// Local
#include "rocket.h"

struct AppState {
  // Rocket
  RocketState rocket_state{};

  // SDL
  SDL_Window *window = nullptr;
  SDL_GPUDevice *gpu_device = nullptr;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

// Called once at startup
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  (void)argc;
  (void)argv;

  AppState *app = new AppState();
  *appstate = app;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    printf("Error: SDL_Init(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  app->window =
      SDL_CreateWindow("SDL Physics Trolling", (int)(1280 * main_scale),
                       (int)(800 * main_scale), window_flags);
  if (!app->window) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetWindowPosition(app->window, SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(app->window);

  app->gpu_device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
          SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB,
      true, nullptr);
  if (!app->gpu_device) {
    printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(app->gpu_device, app->window)) {
    printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetGPUSwapchainParameters(app->gpu_device, app->window,
                                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                SDL_GPU_PRESENTMODE_VSYNC);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot3D::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLGPU(app->window);
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = app->gpu_device;
  init_info.ColorTargetFormat =
      SDL_GetGPUSwapchainTextureFormat(app->gpu_device, app->window);
  init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
  init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
  ImGui_ImplSDLGPU3_Init(&init_info);

  // Load fonts
  style.FontSizeBase = 18.0f;
  io.Fonts->AddFontFromFileTTF("./assets/fonts/Geist-Medium.ttf");
  ImFont *font =
      io.Fonts->AddFontFromFileTTF("./assets/fonts/Geist-Medium.ttf");
  IM_ASSERT(font != nullptr);

  return SDL_APP_CONTINUE;
}

// Called for every SDL event
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  AppState *app = static_cast<AppState *>(appstate);

  ImGui_ImplSDL3_ProcessEvent(event);

  if (event->type == SDL_EVENT_QUIT)
    return SDL_APP_SUCCESS; // clean exit

  if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
      event->window.windowID == SDL_GetWindowID(app->window))
    return SDL_APP_SUCCESS;

  return SDL_APP_CONTINUE;
}

// Called every frame
SDL_AppResult SDL_AppIterate(void *appstate) {
  AppState *app = static_cast<AppState *>(appstate);

  if (SDL_GetWindowFlags(app->window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return SDL_APP_CONTINUE;
  }

  /*
   * Tick Simulation
   */

  /*
   * UI Logic
   */
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // Render Physics Telemetry
  render_rocket_telemetry(&app->rocket_state);

  /*
   * Rendering
   */
  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  const bool minimized =
      (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(app->gpu_device);

  SDL_GPUTexture *swapchain_texture = nullptr;
  SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app->window,
                                        &swapchain_texture, nullptr, nullptr);

  if (swapchain_texture && !minimized) {
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = swapchain_texture;
    target_info.clear_color =
        SDL_FColor{app->clear_color.x, app->clear_color.y, app->clear_color.z,
                   app->clear_color.w};
    target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    target_info.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *render_pass =
        SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
    SDL_EndGPURenderPass(render_pass);
  }

  SDL_SubmitGPUCommandBuffer(command_buffer);
  return SDL_APP_CONTINUE;
}

// Called once on exit
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  (void)result;
  AppState *app = static_cast<AppState *>(appstate);

  SDL_WaitForGPUIdle(app->gpu_device);
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImPlot3D::DestroyContext();
  ImGui::DestroyContext();

  SDL_ReleaseWindowFromGPUDevice(app->gpu_device, app->window);
  SDL_DestroyGPUDevice(app->gpu_device);
  SDL_DestroyWindow(app->window);
  SDL_Quit();

  delete app;
}
