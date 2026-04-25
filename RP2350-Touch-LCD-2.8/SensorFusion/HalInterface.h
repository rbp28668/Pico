#pragma once
#include <cstdint>

namespace sf {

// Platform-abstraction layer. Implement this for your I2C/SPI bus.
class HalInterface {
public:
    virtual ~HalInterface() = default;

    virtual bool writeRegister(uint8_t devAddr, uint8_t reg, uint8_t value) = 0;
    virtual bool readRegister(uint8_t devAddr, uint8_t reg, uint8_t* value) = 0;
    virtual bool burstRead(uint8_t devAddr, uint8_t startReg,
                           uint8_t* buffer, uint8_t length) = 0;

    // Monotonic millisecond timestamp
    virtual uint32_t millis() = 0;
};

} // namespace sf
