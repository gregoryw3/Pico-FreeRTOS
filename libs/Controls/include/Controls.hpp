#pragma once
#include <cstdint>
#include <type_traits>
#include <libfixmath/libfixmath/fix16.hpp>
#include <libfixmath/libfixmath/fix16.h>

namespace Controls {
    /**
     * @brief PID Controller with Generic Type Support
     * 
     */
    namespace PID {
        enum class Mode {
            MANUAL,
            AUTOMATIC
        };
        enum class Type {
            FORWARD_EULER,
            TRAPEZOIDAL
        };

        template<typename T>
        class FloatController {
            static_assert(std::is_floating_point<T>::value, "T must be a floating-point type");
        public:
            FloatController(T kp, T ki, T kd, T sample_time = 1, T gamma = 1, bool enable_back_calculation = true)
                : kp_(kp), ki_(ki), kd_(kd), prev_error_(0), integral_(0), sample_time_(sample_time),
                  gamma_(gamma), enable_back_calculation_(enable_back_calculation) {}
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

                T error;
                T derivative;
                T unclamped_output;
                T output;
                error = setpoint - measured_value;
                if (type_ == Type::TRAPEZOIDAL) {
                    integral_ += ((error + prev_error_) / 2) * dt;
                } else {
                    integral_ += error * dt;
                }
                integral_ += error * dt;
                derivative = (error - prev_error_) / dt;
                prev_error_ = error;
                unclamped_output = kp_ * error + ki_ * integral_ + kd_ * derivative;
                output = unclamped_output;
                if (output_min_ != std::numeric_limits<T>::lowest() && output_max_ != std::numeric_limits<T>::max()) {
                    if (output < output_min_) output = output_min_;
                    if (output > output_max_) output = output_max_;
                }
                if (enable_back_calculation_ && output != unclamped_output) {
                    integral_ -= (unclamped_output - output) * dt / gamma_;
                }
                last_output_ = output;
                return output;
            }

            void setGains(T kp, T ki, T kd) {
                kp_ = kp;
                ki_ = ki;
                kd_ = kd;
            }

            void setKp(T kp) { kp_ = kp; }
            void setKi(T ki) { ki_ = ki; }
            void setKd(T kd) { kd_ = kd; }
            void setGamma(T gamma) { gamma_ = gamma; }

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
            Mode getMode() const { return mode_; }

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
            T sample_time_;
            T gamma_;
            Mode mode_ = Mode::AUTOMATIC; // Default to automatic mode
            Type type_ = Type::FORWARD_EULER; // Default to Forward Euler method
            T output_min_ = std::numeric_limits<T>::lowest();
            T output_max_ = std::numeric_limits<T>::max();
            T last_output_ = 0; // Last output value for integral windup prevention
            bool enable_back_calculation_ = true; // Enable back-calculation for integral windup prevention
        };

        template<typename T>
        class FixedController {
            static_assert(std::is_floating_point<T>::value, "T must be a floating-point type");
        public:
            FixedController(T kp, T ki, T kd, T sample_time = 1, T gamma = 1, bool enable_back_calculation = true)
                : mode_(Mode::AUTOMATIC), type_(Type::FORWARD_EULER), enable_back_calculation_(enable_back_calculation) {
                kp_ = fix16_from_float(static_cast<float>(kp));
                ki_ = fix16_from_float(static_cast<float>(ki));
                kd_ = fix16_from_float(static_cast<float>(kd));
                sample_time_ = fix16_from_float(static_cast<float>(sample_time));
                gamma_ = fix16_from_float(static_cast<float>(gamma));
                prev_error_ = 0;
                integral_ = 0;
                output_min_ = fix16_from_float(-32768.0f);
                output_max_ = fix16_from_float(32767.0f);
                last_output_ = 0;
            }

            T update(T setpoint, T measured_value) {
                return update(setpoint, measured_value, static_cast<T>(fix16_to_float(sample_time_)));
            }

            T update(T setpoint, T measured_value, T dt) {
                if (mode_ == Mode::MANUAL) {
                    return last_output_float_;
                }
                fix16_t setpoint_fx = fix16_from_float(static_cast<float>(setpoint));
                fix16_t measured_fx = fix16_from_float(static_cast<float>(measured_value));
                fix16_t dt_fx = fix16_from_float(static_cast<float>(dt));
                fix16_t error = fix16_sub(setpoint_fx, measured_fx);
                fix16_t derivative;
                if (type_ == Type::TRAPEZOIDAL) {
                    fix16_t sum_err = fix16_add(error, prev_error_);
                    fix16_t avg_err = fix16_div(sum_err, fix16_from_float(2.0f));
                    fix16_t delta_int = fix16_mul(avg_err, dt_fx);
                    integral_ = fix16_add(integral_, delta_int);
                } else {
                    fix16_t delta_int = fix16_mul(error, dt_fx);
                    integral_ = fix16_add(integral_, delta_int);
                }
                derivative = fix16_div(fix16_sub(error, prev_error_), dt_fx);
                prev_error_ = error;
                fix16_t p_term = fix16_mul(kp_, error);
                fix16_t i_term = fix16_mul(ki_, integral_);
                fix16_t d_term = fix16_mul(kd_, derivative);
                fix16_t unclamped_output = fix16_add(fix16_add(p_term, i_term), d_term);
                fix16_t output = unclamped_output;
                if (output < output_min_) output = output_min_;
                if (output > output_max_) output = output_max_;
                if (enable_back_calculation_ && output != unclamped_output) {
                    fix16_t diff = fix16_sub(unclamped_output, output);
                    fix16_t adj = fix16_div(fix16_mul(diff, dt_fx), gamma_);
                    integral_ = fix16_sub(integral_, adj);
                }
                last_output_ = output;
                last_output_float_ = static_cast<T>(fix16_to_float(output));
                return last_output_float_;
            }

            void setGains(T kp, T ki, T kd) {
                kp_ = fix16_from_float(static_cast<float>(kp));
                ki_ = fix16_from_float(static_cast<float>(ki));
                kd_ = fix16_from_float(static_cast<float>(kd));
            }
            void setKp(T kp) { kp_ = fix16_from_float(static_cast<float>(kp)); }
            void setKi(T ki) { ki_ = fix16_from_float(static_cast<float>(ki)); }
            void setKd(T kd) { kd_ = fix16_from_float(static_cast<float>(kd)); }
            void setGamma(T gamma) { gamma_ = fix16_from_float(static_cast<float>(gamma)); }

            T getKp() const { return static_cast<T>(fix16_to_float(kp_)); }
            T getKi() const { return static_cast<T>(fix16_to_float(ki_)); }
            T getKd() const { return static_cast<T>(fix16_to_float(kd_)); }
            T getGamma() const { return static_cast<T>(fix16_to_float(gamma_)); }

            void setOutputLimits(T min, T max) {
                output_min_ = fix16_from_float(static_cast<float>(min));
                output_max_ = fix16_from_float(static_cast<float>(max));
            }
            void setSampleTime(T sample_time) {
                sample_time_ = fix16_from_float(static_cast<float>(sample_time));
            }
            T getSampleTime() const { return static_cast<T>(fix16_to_float(sample_time_)); }

            void setMode(Mode mode) { mode_ = mode; }
            Mode getMode() const { return mode_; }

            void setManualOutput(T output) {
                setMode(Mode::MANUAL);
                last_output_float_ = output;
                last_output_ = fix16_from_float(static_cast<float>(output));
            }

            void enableBackCalculation() { enable_back_calculation_ = true; }
            void disableBackCalculation() { enable_back_calculation_ = false; }

            void setType(Type type) { type_ = type; }
            Type getType() const { return type_; }

            void reset() {
                prev_error_ = 0;
                integral_ = 0;
                last_output_ = 0;
                last_output_float_ = 0;
            }

        private:
            fix16_t kp_, ki_, kd_;
            fix16_t prev_error_, integral_;
            fix16_t sample_time_;
            fix16_t gamma_;
            Mode mode_;
            Type type_;
            fix16_t output_min_;
            fix16_t output_max_;
            fix16_t last_output_;
            T last_output_float_ = 0;
            bool enable_back_calculation_;
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