#pragma once
#include <cstdint>
#include <type_traits>

extern "C" {
    #include <fix16.h>
    #include <fixmatrix.h>
}

namespace Controls {
    /**
     * @brief PID Controller with Generic Type Support
     * 
     */
    namespace PID {
        template<typename T>
        class Controller {
        public:
            enum class Mode {
                // MANUAL mode allows direct output setting
                MANUAL,
                // AUTOMATIC mode uses PID calculations
                AUTOMATIC
            };
            enum class Type {
                FORWARD_EULER,
                TRAPEZOIDAL
            };
            Controller(T kp, T ki, T kd, T scale = 1, T sample_time = 1, T gamma = 1, bool enable_back_calculation = true)
                : kp_(kp), ki_(ki), kd_(kd), scale_(scale), prev_error_(0), integral_(0), sample_time_(sample_time),
                    gamma_(gamma), enable_back_calculation_(enable_back_calculation) {
                    if constexpr (std::is_integral<T>::value) {
                        // Ensure scale is non-zero for integer types
                        if (scale_ == 0) {
                            scale_ = 1;
                        }
                        kp_ = kp * scale_;
                        ki_ = ki * scale_;
                        kd_ = kd * scale_;
                        gamma_ = gamma * scale_;
                    } else {
                        kp_ = kp;
                        ki_ = ki;
                        kd_ = kd;
                        gamma_ = gamma;
                    }
                }
            /**
             * @brief Update the PID controller with a setpoint and measured value, using the constant sample time.
             * 
             * @param setpoint 
             * @param measured_value 
             * @return T 
             */
            T update(T setpoint, T measured_value) {
                return update(setpoint, measured_value, sample_time_);
            }
            /**
             * @brief Update the PID controller with a setpoint, measured value, and time step.
             * 
             * @param setpoint 
             * @param measured_value 
             * @param dt Time step in seconds
             * @return T 
             */
            T update(T setpoint, T measured_value, T dt) {
                if (mode_ == Mode::MANUAL) {
                    return last_output_; // Return last output if in manual mode
                }

                T error = setpoint - measured_value;

                // Trapezoidal integration for the integral term
                if (type_ == Type::TRAPEZOIDAL) {
                    // Average the current and previous error for trapezoidal integration
                    integral_ += ((error + prev_error_) / 2) * dt;
                } else { // Forward Euler
                    integral_ += error * dt;
                }

                integral_ += error * dt;

                T derivative = (error - prev_error_) / dt;
                prev_error_ = error;

                T unclamped_output;
                if constexpr (std::is_integral<T>::value) {
                    // Scale gains and output for integer types
                    unclamped_output = ((kp_ * error) + (ki_ * integral_) + (kd_ * derivative)) / scale_;
                } else {
                    // No scaling for floating-point types
                    unclamped_output = kp_ * error + ki_ * integral_ + kd_ * derivative;
                }

                // Clamp output to limits
                T output = unclamped_output;
                // Check if output limits are set
                if (output_min_ != std::numeric_limits<T>::lowest() && output_max_ != std::numeric_limits<T>::max()) {
                    if (output < output_min_) output = output_min_;
                    if (output > output_max_) output = output_max_;
                }

                if (enable_back_calculation_ && output != unclamped_output) {
                    // Back-calculation for integral windup prevention
                    // gamma_ is already scaled for integer types
                    integral_ -= (unclamped_output - output) * dt / gamma_;
                }

                last_output_ = output;
                return output;
            }

            void setGains(T kp, T ki, T kd) {
                if constexpr (std::is_integral<T>::value) {
                    // Scale gains for integer types
                    kp_ = kp * scale_;
                    ki_ = ki * scale_;
                    kd_ = kd * scale_;
                } else {
                    kp_ = kp;
                    ki_ = ki;
                    kd_ = kd;
                }
            }

            void setKp(T kp) { kp_ = std::is_integral<T>::value ? kp * scale_ : kp; }
            void setKi(T ki) { ki_ = std::is_integral<T>::value ? ki * scale_ : ki; }
            void setKd(T kd) { kd_ = std::is_integral<T>::value ? kd * scale_ : kd; }
            void setGamma(T gamma) { gamma_ = std::is_integral<T>::value ? gamma * scale_ : gamma; }

            T getKp() const { return kp_; }
            T getKi() const { return ki_; }
            T getKd() const { return kd_; }
            T getGamma() const { return gamma_; }

            void setOutputLimits(T min, T max) {
                output_min_ = min;
                output_max_ = max;
            }
            void setSampleTime(T sample_time) {
                sample_time_ = sample_time;
            }
            T getSampleTime() const { return sample_time_; }

            void setMode(Mode mode) { mode_ = mode; }
            Mode getMode() const { return mode_ == Mode::AUTOMATIC; }

            void setManualOutput(T output) {
                // Set the controller to manual mode and store the output
                setMode(Mode::MANUAL);
                last_output_ = output;
            }

            void enableBackCalculation() { enable_back_calculation_ = true; }
            void disableBackCalculation() { enable_back_calculation_ = false; }

            void setType(Type type) { type_ = type; }
            Type getType() const { return type_; }

            void reset() {
                prev_error_ = 0;
                integral_ = 0;
                last_output_ = 0;
            }

            
        private:
            T kp_, ki_, kd_;
            T prev_error_, integral_;
            T scale_;
            T sample_time_;
            T gamma_;
            Mode mode_ = Mode::AUTOMATIC; // Default to automatic mode
            Type type_ = Type::FORWARD_EULER; // Default to Forward Euler method
            T output_min_ = std::numeric_limits<T>::lowest();
            T output_max_ = std::numeric_limits<T>::max();
            T last_output_ = 0; // Last output value for integral windup prevention
            bool enable_back_calculation_ = true; // Enable back-calculation for integral windup prevention
        };
    }

    namespace Luenberger {
        
    }

    namespace Filters {
        namespace LowPass {
            // Low-pass filter implementation
        }
        namespace HighPass {
            // High-pass filter implementation
        }
        namespace BandPass {
            // Band-pass filter implementation
        }
        namespace Notch {
            // Notch filter implementation
        }
        namespace Complementary {
            // Complementary filter implementation
        }

    }
}