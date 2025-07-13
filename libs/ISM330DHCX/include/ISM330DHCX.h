#pragma once

namespace ISM330DHCX {

class ISM330DHCX {
public:
    ISM330DHCX();
    void initialize();
    void read_data();
};

} // namespace ISM330DHCX
