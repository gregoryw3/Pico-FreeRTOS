#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "Controls.h"

int main() {
    using namespace Controls::PID;
    const int steps = 100;
    float setpoint = 10.0;
    float dt = 0.1;
    float process_gain = 0.8;
    float process_noise = 0.1;

    // float PID controllers
    float kp = 2.0;
    float ki = 0.5;
    float kd = 0.1;
    float gamma = 1.0/kp;
    FloatController<float> pid_bc(kp, ki, kd, dt, gamma, true);
    pid_bc.setOutputLimits(-5.0, 5.0);
    pid_bc.enableBackCalculation();

    FloatController<float> pid_nobc(kp, ki, kd, dt, gamma, false);
    pid_nobc.setOutputLimits(-5.0, 5.0);
    pid_nobc.disableBackCalculation();

    // Fixed-point PID controllers (now using float interface)
    FixedController<float> pid_bc_fx(kp, ki, kd, dt, gamma, true);
    pid_bc_fx.setOutputLimits(-5.0, 5.0);
    pid_bc_fx.enableBackCalculation();

    FixedController<float> pid_nobc_fx(kp, ki, kd, dt, gamma, false);
    pid_nobc_fx.setOutputLimits(-5.0, 5.0);
    pid_nobc_fx.disableBackCalculation();

    std::vector<float> time;
    std::vector<float> measured_bc, measured_nobc, output_bc, output_nobc;
    std::vector<float> measured_bc_fx, measured_nobc_fx, output_bc_fx, output_nobc_fx;
    float process_bc = 0.0, process_nobc = 0.0;
    float process_bc_fx = 0.0, process_nobc_fx = 0.0;

    for (int i = 0; i < steps; ++i) {
        // float controllers
        float control_bc = pid_bc.update(setpoint, process_bc, dt);
        process_bc += control_bc * process_gain * dt;
        process_bc += ((rand() % 100) / 100.0 - 0.5) * process_noise;
        measured_bc.push_back(process_bc);
        output_bc.push_back(control_bc);

        float control_nobc = pid_nobc.update(setpoint, process_nobc, dt);
        process_nobc += control_nobc * process_gain * dt;
        process_nobc += ((rand() % 100) / 100.0 - 0.5) * process_noise;
        measured_nobc.push_back(process_nobc);
        output_nobc.push_back(control_nobc);

        // Fixed-point controllers (using float interface)
        float control_bc_fx = pid_bc_fx.update(setpoint, process_bc_fx, dt);
        process_bc_fx += control_bc_fx * process_gain * dt;
        process_bc_fx += ((rand() % 100) / 100.0 - 0.5) * process_noise;
        measured_bc_fx.push_back(process_bc_fx);
        output_bc_fx.push_back(control_bc_fx);

        float control_nobc_fx = pid_nobc_fx.update(setpoint, process_nobc_fx, dt);
        process_nobc_fx += control_nobc_fx * process_gain * dt;
        process_nobc_fx += ((rand() % 100) / 100.0 - 0.5) * process_noise;
        measured_nobc_fx.push_back(process_nobc_fx);
        output_nobc_fx.push_back(control_nobc_fx);

        time.push_back(i * dt);
    }

    // Setup ImGui + ImPlot + GLFW
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1000, 600, "PID Back Calculation Comparison", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("PID Back Calculation Comparison");
        char param_label_left[128], param_label_right[128];
        snprintf(param_label_left, sizeof(param_label_left),
            "Kp: %.2f\nKi: %.2f\nKd: %.2f\nGamma (1/Kp): %.2f\nSetpoint: %.2f",
            kp, ki, kd, gamma, setpoint);
        snprintf(param_label_right, sizeof(param_label_right),
            "dt: %.2f\nProcess Gain: %.2f\nProcess Noise: %.2f\nOutput Limits: [%.2f, %.2f]",
            dt, process_gain, process_noise, -5.0, 5.0);
        ImGui::Columns(2, "param_cols");
        ImGui::TextUnformatted(param_label_left);
        ImGui::NextColumn();
        ImGui::TextUnformatted(param_label_right);
        ImGui::Columns(1);
        float t_min = time.front();
        float t_max = time.back();
        float y_min = std::min({
            *std::min_element(measured_bc.begin(), measured_bc.end()),
            *std::min_element(measured_nobc.begin(), measured_nobc.end()),
            *std::min_element(measured_bc_fx.begin(), measured_bc_fx.end()),
            *std::min_element(measured_nobc_fx.begin(), measured_nobc_fx.end())
        });
        float y_max = std::max({
            *std::max_element(measured_bc.begin(), measured_bc.end()),
            *std::max_element(measured_nobc.begin(), measured_nobc.end()),
            *std::max_element(measured_bc_fx.begin(), measured_bc_fx.end()),
            *std::max_element(measured_nobc_fx.begin(), measured_nobc_fx.end())
        });
        y_min = std::min(y_min, setpoint) - 3.0;
        y_max = std::max(y_max, setpoint) + 3.0;

        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Float: With Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (BC)", time.data(), measured_bc.data(), (int)measured_bc.size());
            ImPlot::PlotLine("Setpoint", time.data(), std::vector<float>(measured_bc.size(), setpoint).data(), (int)measured_bc.size());
            ImPlot::PlotLine("Control (BC)", time.data(), output_bc.data(), (int)output_bc.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Float: Without Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (No BC)", time.data(), measured_nobc.data(), (int)measured_nobc.size());
            ImPlot::PlotLine("Setpoint", time.data(), std::vector<float>(measured_nobc.size(), setpoint).data(), (int)measured_nobc.size());
            ImPlot::PlotLine("Control (No BC)", time.data(), output_nobc.data(), (int)output_nobc.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Fixed: With Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (BC, Fixed)", time.data(), measured_bc_fx.data(), (int)measured_bc_fx.size());
            ImPlot::PlotLine("Setpoint", time.data(), std::vector<float>(measured_bc_fx.size(), setpoint).data(), (int)measured_bc_fx.size());
            ImPlot::PlotLine("Control (BC, Fixed)", time.data(), output_bc_fx.data(), (int)output_bc_fx.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Fixed: Without Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (No BC, Fixed)", time.data(), measured_nobc_fx.data(), (int)measured_nobc_fx.size());
            ImPlot::PlotLine("Setpoint", time.data(), std::vector<float>(measured_nobc_fx.size(), setpoint).data(), (int)measured_nobc_fx.size());
            ImPlot::PlotLine("Control (No BC, Fixed)", time.data(), output_nobc_fx.data(), (int)output_nobc_fx.size());
            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
