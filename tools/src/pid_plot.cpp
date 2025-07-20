#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include "Controls.h"

int main() {
    using namespace Controls::PID;
    // Simulation state
    std::vector<float> time, measured, control_output;
    float setpoint = 10.0f, process = 0.0f, dt = 0.1f, process_gain = 0.8f, process_noise = 0.1f;
    float Kp = 2.0f, Ki = 0.5f, Kd = 0.1f, gamma = 1.0f, lim = 1.0f;
    Controller<float> pid(Kp, Ki, Kd, gamma, dt, lim, true);
    pid.setOutputLimits(-20.0f, 20.0f);
    int step = 0;

    // Setup ImGui + ImPlot + GLFW
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "PID Plot", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    #include <thread>
    const double target_frame_time = 1.0 / 30.0; // 30 FPS
    while (!glfwWindowShouldClose(window)) {
        double frame_start = glfwGetTime();
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("PID Simulation");
        ImGui::SliderFloat("Setpoint", &setpoint, -25.0f, 25.0f, "%.5f", ImGuiSliderFlags_NoInput);
        ImGui::SliderFloat("Kp", &Kp, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
        ImGui::SliderFloat("Ki", &Ki, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
        ImGui::SliderFloat("Kd", &Kd, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
        ImGui::SliderFloat("Gamma (Back-calc)", &gamma, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
        ImGui::SliderFloat("Process Gain", &process_gain, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
        ImGui::SliderFloat("Process Noise", &process_noise, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
        static float sim_update_interval = 0.033f; // 33ms ~ 30Hz
        ImGui::SliderFloat("Simulation Update Interval (s)", &sim_update_interval, 0.01f, 0.5f, "%.3f s", ImGuiSliderFlags_NoInput);
        static bool auto_tune = false;
        static bool back_calc_enabled = true;
        static bool manual_mode = false;
        static float manual_output = 0.0f;
        static bool sweep_setpoint = false;
        static float sweep_amplitude = 10.0f;
        static float sweep_period = 10.0f;
        if (ImGui::Button(sweep_setpoint ? "Stop Setpoint Sweep" : "Start Setpoint Sweep")) {
            sweep_setpoint = !sweep_setpoint;
            // Optionally reset simulation when starting sweep
            // time.clear(); measured.clear(); control.clear(); process = 0.0f; step = 0;
        }
        if (sweep_setpoint) {
            ImGui::SliderFloat("Sweep Amplitude", &sweep_amplitude, 1.0f, 20.0f, "%.2f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("Sweep Period (s)", &sweep_period, 1.0f, 30.0f, "%.2f", ImGuiSliderFlags_NoInput);
            setpoint = sweep_amplitude * std::sin(2.0f * 3.14159265f * (step * dt) / sweep_period);
        }
        if (ImGui::Button(manual_mode ? "Switch to Automatic" : "Switch to Manual")) {
            manual_mode = !manual_mode;
            if (manual_mode) {
                pid.setMode(Controller<float>::Mode::MANUAL);
                pid.setManualOutput(manual_output);
            } else {
                pid.setMode(Controller<float>::Mode::AUTOMATIC);
            }
            time.clear(); measured.clear(); control_output.clear();
            process = 0.0f; step = 0;
        }
        if (manual_mode) {
            ImGui::SliderFloat("Manual Output", &manual_output, -20.0f, 20.0f, "%.1f", ImGuiSliderFlags_NoInput);
            pid.setManualOutput(manual_output);
        }
        if (ImGui::Button(back_calc_enabled ? "Disable Back Calculation" : "Enable Back Calculation")) {
            back_calc_enabled = !back_calc_enabled;
            if (back_calc_enabled) {
                pid.enableBackCalculation();
            } else {
                pid.disableBackCalculation();
            }
            time.clear(); measured.clear(); control_output.clear();
            process = 0.0f; step = 0;
        }
        if (ImGui::Button("Auto Tune PID")) {
            auto_tune = true;
        }
        if (ImGui::Button("Reset")) {
            time.clear(); measured.clear(); control_output.clear();
            process = 0.0f; step = 0;
            pid = Controller<float>(Kp, Ki, Kd, gamma, dt, lim, back_calc_enabled);
            pid.setOutputLimits(-20.0f, 20.0f);
            if (back_calc_enabled) {
                pid.enableBackCalculation();
            } else {
                pid.disableBackCalculation();
            }
            if (manual_mode) {
                pid.setMode(Controller<float>::Mode::MANUAL);
                pid.setManualOutput(manual_output);
            } else {
                pid.setMode(Controller<float>::Mode::AUTOMATIC);
            }
        }
        // Auto tune logic
        if (auto_tune) {
            float best_Kp = Kp, best_Ki = Ki, best_Kd = Kd;
            float best_score = FLT_MAX;
            for (float test_Kp = 0.5f; test_Kp <= 5.0f; test_Kp += 0.5f) {
                for (float test_Ki = 0.0f; test_Ki <= 2.0f; test_Ki += 0.2f) {
                    for (float test_Kd = 0.0f; test_Kd <= 1.0f; test_Kd += 0.1f) {
                        Controller<float> test_pid(test_Kp, test_Ki, test_Kd, gamma, dt, lim, back_calc_enabled);
                        test_pid.setOutputLimits(-20.0f, 20.0f);
                        if (back_calc_enabled) {
                            test_pid.enableBackCalculation();
                        } else {
                            test_pid.disableBackCalculation();
                        }
                        float test_process = 0.0f;
                        float test_score = 0.0f;
                        for (int i = 0; i < 100; ++i) {
                            float test_ctrl = test_pid.update(setpoint, test_process, dt);
                            test_process += test_ctrl * process_gain * dt;
                            test_process += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
                            test_score += std::abs(test_process - setpoint);
                        }
                        if (test_score < best_score) {
                            best_score = test_score;
                            best_Kp = test_Kp;
                            best_Ki = test_Ki;
                            best_Kd = test_Kd;
                        }
                    }
                }
            }
            Kp = best_Kp;
            Ki = best_Ki;
            Kd = best_Kd;
            time.clear(); measured.clear(); control_output.clear();
            process = 0.0f; step = 0;
            pid = Controller<float>(Kp, Ki, Kd, gamma, dt, lim, back_calc_enabled);
            pid.setOutputLimits(-20.0f, 20.0f);
            if (back_calc_enabled) {
                pid.enableBackCalculation();
            } else {
                pid.disableBackCalculation();
            }
            auto_tune = false;
        }
        // Update PID parameters if changed
        pid.setKp(Kp);
        pid.setKi(Ki);
        pid.setKd(Kd);
        pid.setGamma(gamma);
        // Simulation update interval logic
        static double last_sim_update = 0.0;
        double now = glfwGetTime();
        if (now - last_sim_update >= sim_update_interval) {
            float output = pid.update(setpoint, process, dt);
            process += output * process_gain * dt;
            process += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
            time.push_back(step * dt);
            measured.push_back(process);
            control_output.push_back(output);
            step++;
            last_sim_update = now;
        }

        // Prepare setpoint vector for overlay
        std::vector<float> setpoint_vec(time.size());
        for (size_t i = 0; i < time.size(); ++i) {
            if (sweep_setpoint) {
                setpoint_vec[i] = sweep_amplitude * std::sin(2.0f * 3.14159265f * (i * dt) / sweep_period);
            } else {
                setpoint_vec[i] = setpoint;
            }
        }

        // Plotting

        float window_size = 10.0f; // seconds to display
        float latest_time = time.empty() ? 0.0f : time.back();
        ImPlot::SetNextAxesLimits(latest_time - window_size, latest_time, -30.0f, 30.0f, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Output vs Response")) {
            ImPlot::PlotLine("Output", time.data(), control_output.data(), (int)control_output.size());
            ImPlot::PlotLine("Response", time.data(), measured.data(), (int)measured.size());
            ImPlot::EndPlot();
        }

        // Subplot: setpoint and Response response overlay
        ImPlot::SetNextAxesLimits(latest_time - window_size, latest_time, -30.0f, 30.0f, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Setpoint vs Response")) {
            ImPlot::PlotLine("Setpoint", time.data(), setpoint_vec.data(), setpoint_vec.size());
            ImPlot::PlotLine("Response", time.data(), measured.data(), measured.size());
            ImPlot::EndPlot();
        }

        // Subplot: zoomed in on control line
        // ImPlot::SetNextAxesLimits(latest_time - 5.0f, latest_time, -20.5f, 20.5f, ImGuiCond_Always);
        // if (ImPlot::BeginPlot("Control (Zoomed)", ImVec2(-1,150))) {
        //     ImPlot::PlotLine("Control", time.data(), control.data(), control.size());
        //     ImPlot::EndPlot();
        // }
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