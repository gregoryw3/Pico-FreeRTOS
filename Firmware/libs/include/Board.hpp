#pragma once

#include "Pose.hpp"
#include <stdio.h>
#include <type_traits>
#include <utility>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/spi.h"

/**
 * @brief When dealing with multiple sensors, this can be used to represent
 *  the board's physical position and orientation relative to the center of mass.
 *  This is useful for applications like robotics or drones where the center of mass
 *  is crucial for stability and control. By using transformations, we can
 *  easily convert between different coordinate frames and adjust sensor data accordingly.
 * 
 * In this case, 'SensorBase' is used to represent each sensor's position and orientation to the board's PCB.
 * 'BoardBase' extends this to represent the board's position and orientation relative to the center of mass of the system.
 * 
 */
class BoardBase {
public:
    Pose pose; // Position/orientation relative to center of mass
};