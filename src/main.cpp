#define SDL_MAIN_USE_CALLBACKS
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

struct AppState {
  SDL_Window *window = nullptr;
  SDL_GPUDevice *gpu_device = nullptr;

  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

static AppState g_app;

// Called once at startup
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  (void)argc;
  (void)argv;
  *appstate = &g_app;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    printf("Error: SDL_Init(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  g_app.window =
      SDL_CreateWindow("SDL Physics Trolling", (int)(1280 * main_scale),
                       (int)(800 * main_scale), window_flags);
  if (!g_app.window) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetWindowPosition(g_app.window, SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(g_app.window);

  g_app.gpu_device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
          SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB,
      true, nullptr);
  if (!g_app.gpu_device) {
    printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(g_app.gpu_device, g_app.window)) {
    printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetGPUSwapchainParameters(g_app.gpu_device, g_app.window,
                                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                SDL_GPU_PRESENTMODE_VSYNC);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLGPU(g_app.window);
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = g_app.gpu_device;
  init_info.ColorTargetFormat =
      SDL_GetGPUSwapchainTextureFormat(g_app.gpu_device, g_app.window);
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
  (void)appstate;

  ImGui_ImplSDL3_ProcessEvent(event);

  if (event->type == SDL_EVENT_QUIT)
    return SDL_APP_SUCCESS; // clean exit

  if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
      event->window.windowID == SDL_GetWindowID(g_app.window))
    return SDL_APP_SUCCESS;

  return SDL_APP_CONTINUE;
}

// Called every frame
SDL_AppResult SDL_AppIterate(void *appstate) {
  (void)appstate;

  if (SDL_GetWindowFlags(g_app.window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return SDL_APP_CONTINUE;
  }

  // Start the Dear ImGui frame
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // Demo window
  if (g_app.show_demo_window)
    ImGui::ShowDemoWindow(&g_app.show_demo_window);

  // Simple "Hello, world!" window
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");
    ImGui::Text("This is some useful text.");
    ImGui::Checkbox("Demo Window", &g_app.show_demo_window);
    ImGui::Checkbox("Another Window", &g_app.show_another_window);
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", (float *)&g_app.clear_color);

    if (ImGui::Button("Button"))
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
  }

  // Another window
  if (g_app.show_another_window) {
    ImGui::Begin("Another Window", &g_app.show_another_window);
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
      g_app.show_another_window = false;
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  const bool minimized =
      (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(g_app.gpu_device);

  SDL_GPUTexture *swapchain_texture = nullptr;
  SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, g_app.window,
                                        &swapchain_texture, nullptr, nullptr);

  if (swapchain_texture && !minimized) {
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = swapchain_texture;
    target_info.clear_color =
        SDL_FColor{g_app.clear_color.x, g_app.clear_color.y,
                   g_app.clear_color.z, g_app.clear_color.w};
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
  (void)appstate;
  (void)result;

  SDL_WaitForGPUIdle(g_app.gpu_device);
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui::DestroyContext();

  SDL_ReleaseWindowFromGPUDevice(g_app.gpu_device, g_app.window);
  SDL_DestroyGPUDevice(g_app.gpu_device);
  SDL_DestroyWindow(g_app.window);
  SDL_Quit();
}
