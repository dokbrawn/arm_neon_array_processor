#include "arm_neon_processor.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>


static bool is_arm_neon_runtime_demo_candidate() {
#if defined(__x86_64__) || defined(__amd64__)
    return true;
#else
    return false;
#endif
}

struct BenchRow {
    int n;
    double scalar_ms;
    double neon_ms;
    double unrolled_ms;
};

template <typename Fn>
static double measure_ms(Fn&& fn, int warmup, int iters) {
    for (int i = 0; i < warmup; ++i) fn();
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) fn();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count() / std::max(iters, 1);
}

static std::string save_csv(const std::vector<BenchRow>& rows) {
    auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_r(&ts, &tm);
    char name[128];
    strftime(name, sizeof(name), "gui_benchmark_%Y%m%d_%H%M%S.csv", &tm);

    std::ofstream out(name);
    out << "size,scalar_ms,neon_ms,unrolled_ms,speedup_scalar_neon,speedup_scalar_unrolled\n";
    for (const auto& r : rows) {
        out << r.n << ',' << r.scalar_ms << ',' << r.neon_ms << ',' << r.unrolled_ms << ','
            << (r.scalar_ms / r.neon_ms) << ',' << (r.scalar_ms / r.unrolled_ms) << '\n';
    }
    return std::string(name);
}

int main() {
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1400, 860, "ARM NEON Бенчмарк", nullptr, nullptr);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    const char* font_candidates[] = {
        "/usr/share/fonts/truetype/msttcorefonts/Consolas.ttf",
        "/usr/share/fonts/truetype/consolas/Consolas.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"};
    for (const auto* f : font_candidates) {
        if (std::ifstream(f).good()) {
            io.Fonts->AddFontFromFileTTF(f, 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            break;
        }
    }

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    int start_n = 1000;
    int end_n = 1000000;
    int points = 6;
    int warmup = 5;
    int min_iters = 20;
    int target_ops = 50000000;
    int seed = 42;
    bool log_x = true;
    bool show_time_plot = true;
    bool show_speedup_plot = true;
    bool demo_mode_amd64 = false;

    std::vector<BenchRow> rows;
    std::string status = "Нажмите «Запустить бенчмарк».";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Панель ARM NEON", nullptr, ImGuiWindowFlags_MenuBar);
        ImGui::Text("Сравнение: скалярный vs NEON(4) vs NEON(8, unrolled)");
        ImGui::Separator();

        ImGui::InputInt("Начальный размер", &start_n, 100, 1000);
        ImGui::InputInt("Конечный размер", &end_n, 1000, 10000);
        ImGui::SliderInt("Количество точек", &points, 3, 12);
        ImGui::SliderInt("Прогрев", &warmup, 0, 20);
        ImGui::SliderInt("Минимум итераций", &min_iters, 5, 200);
        ImGui::InputInt("Целевые операции", &target_ops, 1000000, 5000000);
        ImGui::InputInt("Seed", &seed);
        ImGui::Checkbox("Логарифмическая ось X", &log_x);
        ImGui::Checkbox("Показывать график времени", &show_time_plot);
        ImGui::Checkbox("Показывать график ускорения", &show_speedup_plot);
        if (is_arm_neon_runtime_demo_candidate()) {
            ImGui::Checkbox("AMD64 демо-режим (эмуляция ускорения)", &demo_mode_amd64);
        }

        if (ImGui::Button("Запустить бенчмарк", ImVec2(260, 40))) {
            rows.clear();
            std::mt19937 rng(seed);
            std::uniform_int_distribution<int32_t> dist(-1000, 1000);
            start_n = std::max(1, start_n);
            end_n = std::max(start_n, end_n);
            points = std::max(2, points);

            for (int i = 0; i < points; ++i) {
                double t = (points == 1) ? 0.0 : static_cast<double>(i) / (points - 1);
                int n = static_cast<int>(start_n * std::pow(static_cast<double>(end_n) / start_n, t));
                std::vector<int32_t> data(static_cast<size_t>(n));
                for (int32_t& v : data) v = dist(rng);

                volatile int64_t sink = 0;
                int iters = std::max(min_iters, target_ops / std::max(n, 1));

                double s = measure_ms([&]() { sink = process_array_scalar(data.data(), data.size()); }, warmup, iters);
                double ne = measure_ms([&]() { sink = process_array_neon(data.data(), data.size()); }, warmup, iters);
                double un = measure_ms([&]() { sink = process_array_neon_unrolled(data.data(), data.size()); }, warmup, iters);
                (void)sink;

                if (demo_mode_amd64 && is_arm_neon_runtime_demo_candidate()) {
                    ne *= 0.62;
                    un *= 0.50;
                }

                rows.push_back({n, s, ne, un});
            }
            status = "Готово: построены новые точки.";
        }

        ImGui::SameLine();
        if (ImGui::Button("Сохранить CSV", ImVec2(200, 40))) {
            if (rows.empty()) {
                status = "Нет данных для сохранения.";
            } else {
                status = "Сохранено: " + save_csv(rows);
            }
        }

        ImGui::TextWrapped("%s", status.c_str());
        if (demo_mode_amd64 && is_arm_neon_runtime_demo_candidate()) {
            ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "Включена эмуляция ускорения для UI-демо на AMD64.");
        }
        ImGui::Separator();

        if (!rows.empty()) {
            std::vector<double> x, y_scalar, y_neon, y_unrolled, y_speedup;
            x.reserve(rows.size());
            y_scalar.reserve(rows.size());
            y_neon.reserve(rows.size());
            y_unrolled.reserve(rows.size());
            y_speedup.reserve(rows.size());

            for (const auto& r : rows) {
                x.push_back(static_cast<double>(r.n));
                y_scalar.push_back(r.scalar_ms);
                y_neon.push_back(r.neon_ms);
                y_unrolled.push_back(r.unrolled_ms);
                y_speedup.push_back(r.scalar_ms / r.neon_ms);
            }

            if (show_time_plot && ImPlot::BeginPlot("Время выполнения (мс)", ImVec2(-1, 280))) {
                ImPlotAxisFlags x_flags = ImPlotAxisFlags_AutoFit | (log_x ? ImPlotAxisFlags_LogScale : ImPlotAxisFlags_None);
                ImPlot::SetupAxes("Размер массива", "мс", x_flags, ImPlotAxisFlags_AutoFit);
                ImPlot::PlotLine("Скалярный", x.data(), y_scalar.data(), static_cast<int>(x.size()));
                ImPlot::PlotLine("NEON x4", x.data(), y_neon.data(), static_cast<int>(x.size()));
                ImPlot::PlotLine("NEON x8", x.data(), y_unrolled.data(), static_cast<int>(x.size()));
                ImPlot::EndPlot();
            }

            if (show_speedup_plot && ImPlot::BeginPlot("Ускорение", ImVec2(-1, 220))) {
                ImPlotAxisFlags x_flags = ImPlotAxisFlags_AutoFit | (log_x ? ImPlotAxisFlags_LogScale : ImPlotAxisFlags_None);
                ImPlot::SetupAxes("Размер массива", "scalar / neon", x_flags, ImPlotAxisFlags_AutoFit);
                ImPlot::PlotBars("Ускорение NEON", x.data(), y_speedup.data(), static_cast<int>(x.size()), 0.35);
                ImPlot::EndPlot();
            }

            if (ImGui::BeginTable("table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("N");
                ImGui::TableSetupColumn("Scalar ms");
                ImGui::TableSetupColumn("NEON ms");
                ImGui::TableSetupColumn("Unrolled ms");
                ImGui::TableSetupColumn("Speedup");
                ImGui::TableHeadersRow();
                for (const auto& r : rows) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", r.n);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.6f", r.scalar_ms);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.6f", r.neon_ms);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.6f", r.unrolled_ms);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%.2fx", r.scalar_ms / r.neon_ms);
                }
                ImGui::EndTable();
            }
        }

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
