/****************************************************************************
 *
 *   Copyright (c) 2020-2021 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file pid_design.hpp
 * @brief Helper functions to design PID controllers using identified models
 *
 * @author Mathieu Bresciani <mathieu@auterion.com>
 */

#include <array>
#include <cmath>
#include <cfloat>

namespace pid_design
{

inline float constrain(float val, float min_val, float max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

// Compute a set of PID gains using General Minimum Variance Control law design
inline std::array<float, 3> computePidGmvc(const std::array<float, 3> &num, const std::array<float, 3> &den, float dt,
                                           float sigma = 0.1f, float delta = 1.f, float lbda = 0.5f)
{
    sigma = constrain(sigma, 0.01f, 1.f);
    delta = constrain(delta, 0.f, 1.f);
    lbda = constrain(lbda, 0.f, 10.f);

    const float a1 = den[1];
    const float a2 = den[2];
    const float b0 = num[0];
    const float b1 = num[1];
    const float b2 = num[2];

    // Solve GMVC law (see derivation in pid_synthesis_symbolic.py)
    const float rho = dt / sigma;
    const float mu = 0.25f * (1.f - delta) + 0.51f * delta; // mu is in the interval [0.25 0.51]
    const float p1 = -2.f * std::exp(-rho / (2.f * mu)) * std::cos(std::sqrt(4.f * mu - 1.f) * rho / (2.f * mu));
    const float p2 = std::exp(-rho / mu);
    const float e1 = -a1 + p1 + 1.f;
    const float f0 = -a1 * e1 + a1 - a2 + e1 + p2;
    const float f1 = a1 * e1 - a2 * e1 + a2;
    const float f2 = a2 * e1;

    // Translate to PID gains
    const float nu = lbda + (e1 + 1.f) * (b0 + b1 + b2);

    if (std::fabs(nu) < FLT_EPSILON) {
        return {0.0f, 0.0f, 0.0f};
    }

    const float kc = -(f1 + 2.f * f2) / nu;
    float ki = -(f0 + f1 + f2) / (dt * (f1 + 2.f * f2));
    ki /= 5.f; // This is not part of the original implementation but is required to produce reasonable gains
    const float kd = -dt * f2 / (f1 + 2.f * f2);

    return {kc, ki, kd};
}
} // namespace pid_design


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
#include <thread>
#include <mutex>
#include <chrono>
#include "Controls.hpp"

int main() {
    using namespace PID;
    // Setpoint will change over time with randomness
    float setpoint_base = 0.0f;
    float setpoint = setpoint_base;
    float dt = 0.001; // 1000 Hz
    const int steps = 10 * 1000; // 10 seconds at 500 Hz
    float process_gain = 0.8;
    float process_noise = 0.1;


    // Example plant coefficients (user should adjust as needed)
    std::array<float, 3> num = {0.1f, 0.05f, 0.01f};
    std::array<float, 3> den = {1.0f, -0.8f, 0.2f};
    float best_gamma = 0.001f;
    auto pid_gains = pid_design::computePidGmvc(num, den, dt);
    float best_kp = pid_gains[0];
    float best_ki = pid_gains[1];
    float best_kd = pid_gains[2];

    // Run simulation with best parameters for plotting
    std::vector<float> sim_time;
    std::vector<float> setpoint_vec; // Store setpoint over time
    std::vector<float> measured_bc, measured_nobc, output_bc, output_nobc;
    std::vector<float> measured_bc_fx, measured_nobc_fx, output_bc_fx, output_nobc_fx;
    float process_bc = 0.0, process_nobc = 0.0;
    float process_bc_fx = 0.0, process_nobc_fx = 0.0;

    FloatController<float> pid_bc(0, 0, 0, dt, best_gamma, true);
    pid_bc.setOutputLimits(-30.0, 30.0);
    pid_bc.enableBackCalculation();

    FloatController<float> pid_nobc(0, 0, 0, dt, best_gamma, false);
    pid_nobc.setOutputLimits(-30.0, 30.0);
    pid_nobc.disableBackCalculation();

    FixedController<float> pid_bc_fx(0, 0, 0, dt, best_gamma, true);
    pid_bc_fx.setOutputLimits(-30.0, 30.0);
    pid_bc_fx.enableBackCalculation();

    FixedController<float> pid_nobc_fx(0, 0, 0, dt, best_gamma, false);
    pid_nobc_fx.setOutputLimits(-30.0, 30.0);
    pid_nobc_fx.disableBackCalculation();

    // Measure float controller simulation time
    auto t_float_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; ++i) {
        if (i % 250 == 0 && i > 0) {
            float delta = ((rand() % 200) / 100.0f - 1.0f) * 1.0f;
            setpoint += delta;
        }
        setpoint += ((rand() % 100) / 100.0f - 0.5f) * 0.05f;
        // Clamp setpoint to [-30, 30]
        setpoint_vec.push_back(setpoint);

        // Recompute PID gains at every step
        auto pid_gains = pid_design::computePidGmvc(num, den, dt);
        pid_bc.setGains(pid_gains[0], pid_gains[1], pid_gains[2]);
        pid_nobc.setGains(pid_gains[0], pid_gains[1], pid_gains[2]);
        pid_bc_fx.setGains(pid_gains[0], pid_gains[1], pid_gains[2]);
        pid_nobc_fx.setGains(pid_gains[0], pid_gains[1], pid_gains[2]);

        float control_bc = pid_bc.update(setpoint, process_bc, dt);
        process_bc += control_bc * process_gain * dt;
        process_bc += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
        measured_bc.push_back(process_bc);
        output_bc.push_back(control_bc);

        float control_nobc = pid_nobc.update(setpoint, process_nobc, dt);
        process_nobc += control_nobc * process_gain * dt;
        process_nobc += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
        measured_nobc.push_back(process_nobc);
        output_nobc.push_back(control_nobc);

        sim_time.push_back(i * dt);
    }
    auto t_float_end = std::chrono::high_resolution_clock::now();
    double float_duration_ms = std::chrono::duration<double, std::milli>(t_float_end - t_float_start).count();

    // Measure fixed-point controller simulation time
    auto t_fixed_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; ++i) {
        float control_bc_fx = pid_bc_fx.update(setpoint_vec[i], process_bc_fx, dt);
        process_bc_fx += control_bc_fx * process_gain * dt;
        process_bc_fx += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
        measured_bc_fx.push_back(process_bc_fx);
        output_bc_fx.push_back(control_bc_fx);

        float control_nobc_fx = pid_nobc_fx.update(setpoint_vec[i], process_nobc_fx, dt);
        process_nobc_fx += control_nobc_fx * process_gain * dt;
        process_nobc_fx += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
        measured_nobc_fx.push_back(process_nobc_fx);
        output_nobc_fx.push_back(control_nobc_fx);
    }
    auto t_fixed_end = std::chrono::high_resolution_clock::now();
    double fixed_duration_ms = std::chrono::duration<double, std::milli>(t_fixed_end - t_fixed_start).count();

    // Output speedup
    printf("Float simulation time: %.3f ms\n", float_duration_ms);
    printf("Fixed-point simulation time: %.3f ms\n", fixed_duration_ms);
    if (fixed_duration_ms > 0.0)
        printf("Fixed-point is %.2fx faster than float.\n", float_duration_ms / fixed_duration_ms);

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
            best_kp, best_ki, best_kd, best_gamma, setpoint);
        snprintf(param_label_right, sizeof(param_label_right),
            "dt: %.4f\nProcess Gain: %.2f\nProcess Noise: %.2f\nOutput Limits: [%.2f, %.2f]",
            dt, process_gain, process_noise, -5.0, 5.0);
        ImGui::Columns(2, "param_cols");
        ImGui::TextUnformatted(param_label_left);
        ImGui::NextColumn();
        ImGui::TextUnformatted(param_label_right);
        ImGui::Columns(1);
        float t_min = sim_time.front();
        float t_max = sim_time.back();
        float y_min = std::min({
            *std::min_element(measured_bc.begin(), measured_bc.end()),
            *std::min_element(measured_nobc.begin(), measured_nobc.end()),
            *std::min_element(measured_bc_fx.begin(), measured_bc_fx.end()),
            *std::min_element(measured_nobc_fx.begin(), measured_nobc_fx.end()),
            *std::min_element(setpoint_vec.begin(), setpoint_vec.end())
        });
        float y_max = std::max({
            *std::max_element(measured_bc.begin(), measured_bc.end()),
            *std::max_element(measured_nobc.begin(), measured_nobc.end()),
            *std::max_element(measured_bc_fx.begin(), measured_bc_fx.end()),
            *std::max_element(measured_nobc_fx.begin(), measured_nobc_fx.end()),
            *std::max_element(setpoint_vec.begin(), setpoint_vec.end())
        });
        y_min -= 3.0;
        y_max += 3.0;

        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Float: With Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (BC)", sim_time.data(), measured_bc.data(), (int)measured_bc.size());
            ImPlot::PlotLine("Setpoint", sim_time.data(), setpoint_vec.data(), (int)setpoint_vec.size());
            // ImPlot::PlotLine("Control (BC)", sim_time.data(), output_bc.data(), (int)output_bc.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Float: Without Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (No BC)", sim_time.data(), measured_nobc.data(), (int)measured_nobc.size());
            ImPlot::PlotLine("Setpoint", sim_time.data(), setpoint_vec.data(), (int)setpoint_vec.size());
            // ImPlot::PlotLine("Control (No BC)", sim_time.data(), output_nobc.data(), (int)output_nobc.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Fixed: With Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (BC, Fixed)", sim_time.data(), measured_bc_fx.data(), (int)measured_bc_fx.size());
            ImPlot::PlotLine("Setpoint", sim_time.data(), setpoint_vec.data(), (int)setpoint_vec.size());
            // ImPlot::PlotLine("Control (BC, Fixed)", sim_time.data(), output_bc_fx.data(), (int)output_bc_fx.size());
            ImPlot::EndPlot();
        }
        ImPlot::SetNextAxesLimits(t_min, t_max, y_min, y_max, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Fixed: Without Back Calculation", ImVec2(-1,200))) {
            ImPlot::PlotLine("Response (No BC, Fixed)", sim_time.data(), measured_nobc_fx.data(), (int)measured_nobc_fx.size());
            ImPlot::PlotLine("Setpoint", sim_time.data(), setpoint_vec.data(), (int)setpoint_vec.size());
            // ImPlot::PlotLine("Control (No BC, Fixed)", sim_time.data(), output_nobc_fx.data(), (int)output_nobc_fx.size());
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
