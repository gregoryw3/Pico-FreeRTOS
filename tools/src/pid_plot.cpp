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
#include "Controls.hpp"

int main() {
    using namespace PID;
    // Simulation state
    std::vector<float> time, measured, control_output, setpoint_history;
    float setpoint = 10.0f, process = 0.0f, dt = 0.1f, process_gain = 0.8f, process_noise = 0.1f;
    float Kp = 2.0f, Ki = 0.5f, Kd = 0.1f, gamma = 1.0f;
    FloatController<float> pid(Kp, Ki, Kd, dt, gamma, true);
    pid.setOutputLimits(-20.0f, 20.0f);
    pid.setAntiWindupMethod(AntiWindup::BACK_CALCULATION); // Use stable method by default
    int step = 0;
    
    // Settling analysis variables
    float previous_setpoint = setpoint;
    float settling_tolerance = 0.02f; // 2% tolerance
    int settling_window = 20; // Number of samples to check for settling
    std::vector<float> setpoint_change_times;
    std::vector<float> settling_times;
    std::vector<float> overshoot_values;
    std::vector<float> overshoot_times;
    float setpoint_change_time = -1.0f;
    bool waiting_for_settling = false;
    float target_setpoint = setpoint;

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
        
        // System Model Selection
        ImGui::Separator();
        ImGui::Text("System Model");
        static int system_model = 0; // 0=Integrator, 1=First Order, 2=Second Order, 3=Motor
        const char* system_models[] = { 
            "Pure Integrator (Position Control)", 
            "First Order (RC Circuit, Temperature)", 
            "Second Order (Mass-Spring-Damper)",
            "DC Motor with Load"
        };
        if (ImGui::Combo("Model", &system_model, system_models, 4)) {
            // Reset when changing models
            process = 0.0f;
            time.clear(); measured.clear(); control_output.clear(); setpoint_history.clear();
            setpoint_change_times.clear(); settling_times.clear();
            overshoot_values.clear(); overshoot_times.clear();
            step = 0;
        }
        
        // Model-specific parameters
        static float time_constant = 1.0f;      // First order tau
        static float natural_freq = 2.0f;       // Second order wn
        static float damping_ratio = 0.7f;      // Second order zeta
        static float motor_inertia = 0.01f;     // Motor J
        static float motor_damping = 0.1f;      // Motor b
        static float motor_kt = 0.1f;           // Motor torque constant
        
        if (system_model == 1) { // First Order
            ImGui::SliderFloat("Time Constant (s)", &time_constant, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_NoInput);
        } else if (system_model == 2) { // Second Order
            ImGui::SliderFloat("Natural Frequency (rad/s)", &natural_freq, 0.5f, 10.0f, "%.2f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("Damping Ratio", &damping_ratio, 0.1f, 2.0f, "%.2f", ImGuiSliderFlags_NoInput);
        } else if (system_model == 3) { // DC Motor
            ImGui::SliderFloat("Motor Inertia (kg⋅m²)", &motor_inertia, 0.001f, 0.1f, "%.4f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("Motor Damping (N⋅m⋅s)", &motor_damping, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::SliderFloat("Torque Constant (N⋅m/A)", &motor_kt, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput);
        }
        
        // Settling analysis controls
        ImGui::Separator();
        ImGui::Text("Settling Analysis");
        ImGui::SliderFloat("Settling Tolerance (%)", &settling_tolerance, 0.001f, 0.1f, "%.3f", ImGuiSliderFlags_NoInput);
        ImGui::SliderInt("Settling Window (samples)", &settling_window, 5, 50);
        
        // Anti-windup method selection
        ImGui::Separator();
        ImGui::Text("Anti-Windup Method");
        static int antiwindup_method = 0; // 0 = BACK_CALCULATION, 1 = CONDITIONAL_INTEGRATION
        const char* antiwindup_methods[] = { "Back Calculation (Stable)", "Conditional Integration (Textbook)" };
        if (ImGui::Combo("Method", &antiwindup_method, antiwindup_methods, 2)) {
            if (antiwindup_method == 0) {
                pid.setAntiWindupMethod(AntiWindup::BACK_CALCULATION);
            } else {
                pid.setAntiWindupMethod(AntiWindup::CONDITIONAL_INTEGRATION);
            }
            // Reset PID to avoid instability when switching methods
            pid.reset();
            time.clear(); measured.clear(); control_output.clear(); setpoint_history.clear();
            setpoint_change_times.clear(); settling_times.clear();
            overshoot_values.clear(); overshoot_times.clear();
            process = 0.0f; step = 0;
        }
        
        static float sim_update_interval = 0.002f; // 2ms = 500Hz
        ImGui::SliderFloat("Simulation Update Interval (s)", &sim_update_interval, 0.001f, 0.1f, "%.4f s (%.0f Hz)", ImGuiSliderFlags_NoInput);
        
        // Show frequency
        float frequency = 1.0f / sim_update_interval;
        ImGui::Text("PID Update Frequency: %.0f Hz", frequency);
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
                pid.setMode(Mode::MANUAL);
                pid.setManualOutput(manual_output);
            } else {
                pid.setMode(Mode::AUTOMATIC);
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
            time.clear(); measured.clear(); control_output.clear(); setpoint_history.clear();
            setpoint_change_times.clear(); settling_times.clear();
            overshoot_values.clear(); overshoot_times.clear();
            process = 0.0f; step = 0;
            previous_setpoint = setpoint;
            waiting_for_settling = false;
            setpoint_change_time = -1.0f;
            target_setpoint = setpoint;
            
            // Preserve the current anti-windup method
            AntiWindup current_method = pid.getAntiWindupMethod();
            
            pid = FloatController<float>(Kp, Ki, Kd, dt, gamma, back_calc_enabled);
            pid.setOutputLimits(-20.0f, 20.0f);
            pid.setAntiWindupMethod(current_method); // Restore the method
            
            if (back_calc_enabled) {
                pid.enableBackCalculation();
            } else {
                pid.disableBackCalculation();
            }
            if (manual_mode) {
                pid.setMode(Mode::MANUAL);
                pid.setManualOutput(manual_output);
            } else {
                pid.setMode(Mode::AUTOMATIC);
            }
        }
        // Auto tune logic
        if (auto_tune) {
            float best_Kp = Kp, best_Ki = Ki, best_Kd = Kd;
            float best_score = FLT_MAX;
            
            // Preserve current anti-windup method
            AntiWindup current_method = pid.getAntiWindupMethod();
            
            for (float test_Kp = 0.5f; test_Kp <= 5.0f; test_Kp += 0.5f) {
                for (float test_Ki = 0.0f; test_Ki <= 2.0f; test_Ki += 0.2f) {
                    for (float test_Kd = 0.0f; test_Kd <= 1.0f; test_Kd += 0.1f) {
                        FloatController<float> test_pid(test_Kp, test_Ki, test_Kd, dt, gamma, back_calc_enabled);
                        test_pid.setOutputLimits(-20.0f, 20.0f);
                        test_pid.setAntiWindupMethod(current_method);
                        
                        if (back_calc_enabled) {
                            test_pid.enableBackCalculation();
                        } else {
                            test_pid.disableBackCalculation();
                        }
                        float test_process = 0.0f;
                        float test_velocity = 0.0f;  // For second-order models
                        float test_score = 0.0f;
                        for (int i = 0; i < 100; ++i) {
                            float test_ctrl = test_pid.update(setpoint, test_process, dt);
                            
                            // Apply same system model as main simulation
                            switch (system_model) {
                                case 0:
                                    test_process += test_ctrl * process_gain * dt;
                                    break;
                                case 1:
                                    test_process += ((process_gain * test_ctrl) - test_process) / time_constant * dt;
                                    break;
                                case 2: {
                                    float acc = (natural_freq * natural_freq * process_gain * test_ctrl) 
                                              - (2.0f * damping_ratio * natural_freq * test_velocity) 
                                              - (natural_freq * natural_freq * test_process);
                                    test_velocity += acc * dt;
                                    test_process += test_velocity * dt;
                                    break;
                                }
                                case 3: {
                                    float torque = motor_kt * test_ctrl;
                                    float acc = (torque - motor_damping * test_velocity) / motor_inertia;
                                    test_velocity += acc * dt;
                                    test_process += test_velocity * dt;
                                    break;
                                }
                            }
                            
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
            
            pid = FloatController<float>(Kp, Ki, Kd, dt, gamma, back_calc_enabled);
            pid.setOutputLimits(-20.0f, 20.0f);
            pid.setAntiWindupMethod(current_method); // Restore the method
            
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
        
        // Sync PID dt with simulation dt to avoid chart jumping
        static float prev_sim_interval = sim_update_interval;
        if (std::abs(sim_update_interval - prev_sim_interval) > 0.0001f) {
            dt = sim_update_interval;
            
            // Recreate PID controller with new dt
            AntiWindup current_method = pid.getAntiWindupMethod();
            pid = FloatController<float>(Kp, Ki, Kd, dt, gamma, back_calc_enabled);
            pid.setOutputLimits(-20.0f, 20.0f);
            pid.setAntiWindupMethod(current_method);
            
            if (back_calc_enabled) {
                pid.enableBackCalculation();
            } else {
                pid.disableBackCalculation();
            }
            if (manual_mode) {
                pid.setMode(Mode::MANUAL);
                pid.setManualOutput(manual_output);
            } else {
                pid.setMode(Mode::AUTOMATIC);
            }
            
            prev_sim_interval = sim_update_interval;
        }
        // Simulation update interval logic
        static double last_sim_update = 0.0;
        double now = glfwGetTime();
        if (now - last_sim_update >= sim_update_interval) {
            float current_setpoint = setpoint;
            if (sweep_setpoint) {
                current_setpoint = sweep_amplitude * std::sin(2.0f * 3.14159265f * (step * dt) / sweep_period);
            }
            
            // Detect setpoint change
            if (std::abs(current_setpoint - previous_setpoint) > 0.1f) {
                setpoint_change_time = step * dt;
                setpoint_change_times.push_back(setpoint_change_time);
                waiting_for_settling = true;
                target_setpoint = current_setpoint;
                previous_setpoint = current_setpoint;
            }
            
            float output = pid.update(current_setpoint, process, dt);
            
            // Safety check for NaN or infinite values
            if (!std::isfinite(output)) {
                output = 0.0f;
                pid.reset();
            }
            
            // Simulation step - different models
            // dt is now synchronized with sim_update_interval
            
            // Apply control signal based on system model
            static float velocity = 0.0f;      // For second-order and motor models
            
            switch (system_model) {
                case 0: // Pure Integrator (Position Control)
                    process += output * process_gain * dt;
                    break;
                    
                case 1: // First Order (RC Circuit, Temperature)
                    // dx/dt = (K*u - x) / tau
                    process += ((process_gain * output) - process) / time_constant * dt;
                    break;
                    
                case 2: { // Second Order (Mass-Spring-Damper)
                    // d²x/dt² + 2*ζ*ωn*dx/dt + ωn²*x = ωn²*K*u
                    float acceleration = (natural_freq * natural_freq * process_gain * output) 
                                       - (2.0f * damping_ratio * natural_freq * velocity) 
                                       - (natural_freq * natural_freq * process);
                    velocity += acceleration * dt;
                    process += velocity * dt;
                    break;
                }
                
                case 3: { // DC Motor with Load
                    // τ = J*dω/dt + b*ω, τ = Kt*i = Kt*u (assuming current control)
                    float torque = motor_kt * output;
                    float acceleration = (torque - motor_damping * velocity) / motor_inertia;
                    velocity += acceleration * dt;
                    process += velocity * dt; // Integrate velocity to get position
                    break;
                }
            }
            
            // Add process noise
            process += ((rand() % 100) / 100.0f - 0.5f) * process_noise;
            
            // Prevent process from going to extreme values
            if (!std::isfinite(process) || std::abs(process) > 1000.0f) {
                process = 0.0f;
                pid.reset();
            }
            
            // Track overshoot
            if (waiting_for_settling && time.size() > 1) {
                float error = std::abs(process - target_setpoint);
                float setpoint_range = std::abs(target_setpoint);
                if (setpoint_range > 0.1f) { // Avoid division by small numbers
                    float overshoot_percent = error / setpoint_range;
                    if (overshoot_percent > 0.05f) { // 5% overshoot threshold
                        // Check if this is a new peak
                        bool is_new_peak = overshoot_times.empty() || 
                                         (step * dt > overshoot_times.back() + 0.5f);
                        if (is_new_peak) {
                            overshoot_times.push_back(step * dt);
                            overshoot_values.push_back(process);
                        }
                    }
                }
            }
            
            // Check for settling
            if (waiting_for_settling && time.size() >= settling_window) {
                bool settled = true;
                float tolerance_band = std::abs(target_setpoint) * settling_tolerance;
                if (tolerance_band < 0.01f) tolerance_band = 0.01f; // Minimum tolerance
                
                // Check last N samples for settling
                for (int i = 0; i < settling_window; ++i) {
                    int idx = (int)measured.size() - 1 - i;
                    if (idx >= 0) {
                        float error = std::abs(measured[idx] - target_setpoint);
                        if (error > tolerance_band) {
                            settled = false;
                            break;
                        }
                    }
                }
                
                if (settled) {
                    float settling_time = (step * dt) - setpoint_change_time;
                    settling_times.push_back(settling_time);
                    waiting_for_settling = false;
                }
            }
            
            time.push_back(step * dt);
            measured.push_back(process);
            control_output.push_back(output);
            setpoint_history.push_back(current_setpoint);
            step++;
            last_sim_update = now;
        }

        // Performance metrics display
        ImGui::Begin("PID Performance Metrics");
        if (!settling_times.empty()) {
            ImGui::Text("Settling Times:");
            for (size_t i = 0; i < settling_times.size(); ++i) {
                ImGui::Text("  Change %zu: %.2f seconds", i + 1, settling_times[i]);
            }
            float avg_settling = 0;
            for (float t : settling_times) avg_settling += t;
            avg_settling /= settling_times.size();
            ImGui::Text("Average Settling Time: %.2f seconds", avg_settling);
        }
        
        if (!overshoot_times.empty()) {
            ImGui::Text("Overshoot Events: %zu", overshoot_times.size());
            float max_overshoot = 0;
            for (size_t i = 0; i < overshoot_values.size(); ++i) {
                float overshoot_percent = 0;
                if (i < setpoint_change_times.size()) {
                    // Find corresponding setpoint for this overshoot
                    float sp = target_setpoint; // Approximation
                    if (sp != 0) {
                        overshoot_percent = std::abs((overshoot_values[i] - sp) / sp) * 100.0f;
                        max_overshoot = std::max(max_overshoot, overshoot_percent);
                    }
                }
            }
            ImGui::Text("Maximum Overshoot: %.1f%%", max_overshoot);
        }
        ImGui::End();

        // Plotting
        float window_size = 10.0f; // seconds to display
        float latest_time = time.empty() ? 0.0f : time.back();
        
        // Calculate adaptive plot ranges
        float min_val = -30.0f, max_val = 30.0f;
        if (!measured.empty() && !control_output.empty()) {
            float process_min = *std::min_element(measured.begin(), measured.end());
            float process_max = *std::max_element(measured.begin(), measured.end());
            float control_min = *std::min_element(control_output.begin(), control_output.end());
            float control_max = *std::max_element(control_output.begin(), control_output.end());
            
            min_val = std::min({process_min, control_min, -25.0f}) - 5.0f;
            max_val = std::max({process_max, control_max, 25.0f}) + 5.0f;
            
            // Limit extreme ranges
            if (min_val < -100.0f) min_val = -100.0f;
            if (max_val > 100.0f) max_val = 100.0f;
        }
        
        // Main plot with markers
        ImPlot::SetNextAxesLimits(latest_time - window_size, latest_time, min_val, max_val, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Output vs Response with Markers")) {
            ImPlot::PlotLine("Output", time.data(), control_output.data(), (int)control_output.size());
            ImPlot::PlotLine("Response", time.data(), measured.data(), (int)measured.size());
            
            // Mark setpoint changes
            if (!setpoint_change_times.empty()) {
                std::vector<float> marker_y;
                for (size_t i = 0; i < setpoint_change_times.size(); ++i) {
                    marker_y.push_back(0.0f); // Place at y=0
                }
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Up, 8, ImVec4(1,0,0,1), 2);
                ImPlot::PlotScatter("Setpoint Changes", setpoint_change_times.data(), marker_y.data(), (int)setpoint_change_times.size());
            }
            
            // Mark overshoot events
            if (!overshoot_times.empty()) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 6, ImVec4(1,1,0,1), 2);
                ImPlot::PlotScatter("Overshoots", overshoot_times.data(), overshoot_values.data(), (int)overshoot_times.size());
            }
            
            ImPlot::EndPlot();
        }

        // Subplot: setpoint and Response overlay with settling indicators
        ImPlot::SetNextAxesLimits(latest_time - window_size, latest_time, min_val, max_val, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Setpoint vs Response with Settling Analysis")) {
            ImPlot::PlotLine("Setpoint", time.data(), setpoint_history.data(), (int)setpoint_history.size());
            ImPlot::PlotLine("Response", time.data(), measured.data(), (int)measured.size());
            
            // Add settling tolerance bands around setpoint
            if (!time.empty() && !setpoint_history.empty()) {
                std::vector<float> upper_band, lower_band;
                for (size_t i = 0; i < time.size(); ++i) {
                    float tolerance_band = std::abs(setpoint_history[i]) * settling_tolerance;
                    if (tolerance_band < 0.01f) tolerance_band = 0.01f;
                    upper_band.push_back(setpoint_history[i] + tolerance_band);
                    lower_band.push_back(setpoint_history[i] - tolerance_band);
                }
                ImPlot::SetNextFillStyle(ImVec4(0, 1, 0, 0.2f));
                ImPlot::PlotShaded("Settling Band", time.data(), lower_band.data(), upper_band.data(), (int)time.size());
            }
            
            // Mark settling points
            if (!setpoint_change_times.empty() && !settling_times.empty()) {
                std::vector<float> settling_points_time, settling_points_y;
                for (size_t i = 0; i < std::min(setpoint_change_times.size(), settling_times.size()); ++i) {
                    settling_points_time.push_back(setpoint_change_times[i] + settling_times[i]);
                    // Find the response value at settling time
                    float settle_time = setpoint_change_times[i] + settling_times[i];
                    for (size_t j = 0; j < time.size(); ++j) {
                        if (time[j] >= settle_time) {
                            settling_points_y.push_back(measured[j]);
                            break;
                        }
                    }
                }
                if (!settling_points_time.empty()) {
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 8, ImVec4(0,1,0,1), 3);
                    ImPlot::PlotScatter("Settled", settling_points_time.data(), settling_points_y.data(), (int)settling_points_time.size());
                }
            }
            
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