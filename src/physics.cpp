#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "implot3d.h"
#include "implot3d_internal.h"

// Utility structure for realtime plot
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<float> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x) {
        if (Data.size() < MaxSize)
            Data.push_back(x);
        else {
            Data[Offset] = x;
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset = 0;
        }
    }
};

void render_physics_window() {
    ImGui::Begin("Realtime Plot");
    ImGui::Text("Move your mouse to change the data!");
    static ScrollingBuffer sdata1, sdata2, sdata3;
    static ImPlot3DAxisFlags flags = ImPlot3DAxisFlags_NoTickLabels;
    static float t = 0.0f;
    static float last_t = -1.0f;

    if (ImPlot3D::BeginPlot("Scrolling Plot")) {
        // Pool mouse data every 10 ms
        t += ImGui::GetIO().DeltaTime;
        if (t - last_t > 0.01f) {
            last_t = t;
            ImVec2 mouse = ImGui::GetMousePos();
            if (ImAbs(mouse.x) < 1e4f && ImAbs(mouse.y) < 1e4f) {
                ImVec2 plot_center = ImPlot3D::GetFramePos();
                plot_center.x += ImPlot3D::GetFrameSize().x / 2;
                plot_center.y += ImPlot3D::GetFrameSize().y / 2;
                sdata1.AddPoint(t);
                sdata2.AddPoint(mouse.x - plot_center.x);
                sdata3.AddPoint(mouse.y - plot_center.y);
            }
        }

        ImPlot3D::SetupAxes("Time", "Mouse X", "Mouse Y", flags, flags, flags);
        ImPlot3D::SetupAxisLimits(ImAxis3D_X, t - 10.0, t, ImPlot3DCond_Always);
        ImPlot3D::SetupAxisLimits(ImAxis3D_Y, -400, 400, ImPlot3DCond_Once);
        ImPlot3D::SetupAxisLimits(ImAxis3D_Z, -400, 400, ImPlot3DCond_Once);
        ImPlot3D::PlotLine("Mouse", &sdata1.Data[0], &sdata2.Data[0], &sdata3.Data[0], sdata1.Data.size(),
                           {ImPlot3DProp_Offset, sdata1.Offset, ImPlot3DProp_Stride, sizeof(float)});
        ImPlot3D::EndPlot();
    }
    ImGui::End();
}
