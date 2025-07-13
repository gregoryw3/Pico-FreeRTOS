#pragma once

#include "Sensor.hpp"

namespace ADXL375 {

template<typename BusType>
class ADXL375 : public Sensor<BusType> {
public:
    using Sensor<BusType>::Sensor;
    void hello() { printf("Hello from ADXL375\n"); }

    // Compile-time check: only allow I2C or SPI
    static_assert(
        std::is_same<BusType, I2C>::value || std::is_same<BusType, SPI>::value,
        "ADXL375 only supports I2C or SPI"
    );
};

} // namespace ADXL375
