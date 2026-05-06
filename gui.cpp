#include "arm_neon_processor.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <GLFW/glfw3.h>

#include <vector>

int main() {
    if (!glfwInit()) return 1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ARM NEON Benchmark", nullptr, nullptr);
    if (!window) return 1;
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::vector<double> n = {1e3, 1e4, 1e5, 1e6, 5e6};
    std::vector<double> speedup = {1.0, 1.2, 1.8, 2.4, 3.1};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Benchmark Dashboard");
        ImGui::Text("Use bench_cli to generate fresh CSV, this window is for visualization.");

        if (ImPlot::BeginPlot("Speedup vs size")) {
            ImPlot::SetupAxes("N", "Speedup");
            ImPlot::PlotLine("scalar/neon", n.data(), speedup.data(), static_cast<int>(n.size()));
            ImPlot::EndPlot();
        }

        ImGui::TextWrapped("Export: run `pandoc benchmark_*.md -o report.pdf` to get PDF report.");
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
