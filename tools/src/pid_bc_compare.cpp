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
    // Simulation parameters
    const int steps = 100;
    double setpoint = 10.0;
    double dt = 0.1;
    double process_gain = 0.8;
    double process_noise = 0.1;

    // PID controller setup
    auto kp = 2.0;
    auto ki = 0.5;
    auto kd = 0.1;
    auto gamma = 1/kp;
    Controller<double> pid_bc(kp, ki, kd, 1.0, dt, gamma, true);
    pid_bc.setOutputLimits(-5.0, 5.0);
    pid_bc.enableBackCalculation();

    Controller<double> pid_nobc(kp, ki, kd, 1.0, dt, gamma, false);
    pid_nobc.setOutputLimits(-5.0, 5.0);
    pid_nobc.disableBackCalculation();

    std::vector<double> time, measured_bc, measured_nobc, output_bc, output_nobc;
    double process_bc = 0.0, process_nobc = 0.0;

    for (int i = 0; i < steps; ++i) {
        double control_bc = pid_bc.update(setpoint, process_bc, dt);
        process_bc += control_bc * process_gain * dt;
        process_bc += ((rand() % 100) / 100.0 - 0.5) * process_noise;
        measured_bc.push_back(process_bc);
        output_bc.push_back(control_bc);

        double control_nobc = pid_nobc.update(setpoint, process_nobc, dt);
        process_nobc += control_nobc * process_gain * dt;
        process_nobc += ((rand() % 100) / 100.0 - 0.5) * process_noise;
        measured_nobc.push_back(process_nobc);
        output_nobc.push_back(control_nobc);

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
            "Kp: %.2f\nKi: %.2f\nKd: %.2f\nGamma: %.2f\nSetpoint: %.2f",
            kp, ki, kd, gamma, setpoint);
        snprintf(param_label_right, sizeof(param_label_right),
            "dt: %.2f\nProcess Gain: %.2f\nProcess Noise: %.2f\nOutput Limits: [%.2f, %.2f]",
            dt, process_gain, process_noise, -5.0, 5.0);
        ImGui::Columns(2, "param_cols");
        ImGui::TextUnformatted(param_label_left);
        ImGui::NextColumn();
        ImGui::TextUnformatted(param_label_right);
        ImGui::Columns(1);
        double t_min = time.front();
        double t_max = time.back();
        double y_min = std::min(*std::min_element(measured_bc.begin(), measured_bc.end()), *std::min_element(measured_nobc.begin(), measured_nobc.end()));
        double y_max = std::max(*std::max_element(measured_bc.begin(), measured_bc.end()), *std::max_element(measured_nobc.begin(), measured_nobc.end()));
        y_min = std::min(y_min, setpoint) - 3.0;
        y_max = std::max(y_max, setpoint) + 3.0;

        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Double: With Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (BC)", time.data(), measured_bc.data(), (int)measured_bc.size());
            ImPlot::PlotLine("Setpoint", time.data(), std::vector<double>(measured_bc.size(), setpoint).data(), (int)measured_bc.size());
            ImPlot::PlotLine("Control (BC)", time.data(), output_bc.data(), (int)output_bc.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Double: Without Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (No BC)", time.data(), measured_nobc.data(), (int)measured_nobc.size());
            ImPlot::PlotLine("Setpoint", time.data(), std::vector<double>(measured_nobc.size(), setpoint).data(), (int)measured_nobc.size());
            ImPlot::PlotLine("Control (No BC)", time.data(), output_nobc.data(), (int)output_nobc.size());
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
