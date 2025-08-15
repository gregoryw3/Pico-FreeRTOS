#pragma once

namespace LSM6DSO32 {

class LSM6DSO32 {
public:
    LSM6DSO32();
    void initialize();
    void read_data();
};

} // namespace LSM6DSO32
