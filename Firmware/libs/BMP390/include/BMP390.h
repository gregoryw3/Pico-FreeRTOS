#pragma once

namespace BMP390 {

class BMP390 {
public:
    BMP390();
    void initialize();
    void read_data();
};

} // namespace BMP390
