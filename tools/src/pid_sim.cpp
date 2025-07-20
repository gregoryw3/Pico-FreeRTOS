#include <iostream>
#include <vector>
#include <cmath>
#include "Controls.h"

int main() {
    using namespace Controls::PID;

    // Simulation parameters
    double setpoint = 10.0;
    double measured = 0.0;
    double process = 0.0;
    double dt = 0.1;
    double process_gain = 0.8;
    double process_noise = 0.1;

    // PID controller setup
    Controller<double> pid(
        2.0, 
        0.5, 
        0.1, 
        1.0, 
        dt, 
        1.0,
        true
    );
    pid.setOutputLimits(-5.0, 5.0);

    std::vector<double> history;

    for (int i = 0; i < 100; ++i) {
        double control = pid.update(setpoint, measured, dt);

        // Simulate process response
        process += control * process_gain * dt;
        process += ((rand() % 100) / 100.0 - 0.5) * process_noise; // Add noise
        measured = process;

        history.push_back(measured);

        std::cout << "Step " << i << ": Control=" << control << " Measured=" << measured << std::endl;
    }

    return 0;
}