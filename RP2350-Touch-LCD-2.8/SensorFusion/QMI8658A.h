#pragma once
#include "HalInterface.h"
#include "Vec3.h"
#include <cstdint>
#include <cmath>

namespace sf {

// ---- Register map -----------------------------------------------------------

namespace QMI_REG {
    constexpr uint8_t WHO_AM_I    = 0x00;
    constexpr uint8_t REVISION_ID = 0x01;
    constexpr uint8_t CTRL2       = 0x03;
    constexpr uint8_t CTRL3       = 0x04;
    constexpr uint8_t CTRL5       = 0x06;
    constexpr uint8_t CTRL7       = 0x08;
    constexpr uint8_t CTRL9       = 0x0A;
    constexpr uint8_t STATUSINT   = 0x2D;
    constexpr uint8_t TIMESTAMP_L = 0x30;
    constexpr uint8_t TEMP_L      = 0x33;
    constexpr uint8_t COD_STATUS  = 0x46;
}

// ---- Configuration enums ----------------------------------------------------

enum class AccelScale : uint8_t {
    G2  = 0x00,   // 16384 LSB/g
    G4  = 0x10,   // 8192
    G8  = 0x20,   // 4096
    G16 = 0x30    // 2048
};

enum class GyroScale : uint8_t {
    DPS16   = 0x00,  // 2048 LSB/dps
    DPS32   = 0x10,  // 1024
    DPS64   = 0x20,  // 512
    DPS128  = 0x30,  // 256
    DPS256  = 0x40,  // 128
    DPS512  = 0x50,  // 64
    DPS1024 = 0x60,  // 32
    DPS2048 = 0x70   // 16
};

enum class AccelODR : uint8_t {
    Hz7174 = 0x00, Hz3587 = 0x01, Hz1794 = 0x02,
    Hz897  = 0x03, Hz448  = 0x04, Hz224  = 0x05,
    Hz112  = 0x06, Hz56   = 0x07, Hz28   = 0x08,
    LP128  = 0x0C, LP21   = 0x0D, LP11   = 0x0E, LP3 = 0x0F
};

enum class GyroODR : uint8_t {
    Hz7174 = 0x00, Hz3587 = 0x01, Hz1794 = 0x02,
    Hz897  = 0x03, Hz448  = 0x04, Hz224  = 0x05,
    Hz112  = 0x06, Hz56   = 0x07, Hz28   = 0x08
};

enum class LPFMode : uint8_t {
    BW_2_66  = 0x00,   // 2.66 % of ODR
    BW_3_63  = 0x01,   // 3.63 %
    BW_5_39  = 0x02,   // 5.39 %
    BW_13_37 = 0x03    // 13.37 %
};

// ---- Output sample ----------------------------------------------------------

struct ImuSample {
    Vec3     accel;          // m/s^2, body frame
    Vec3     gyro;           // rad/s, body frame
    float    temperature;    // Celsius
    uint32_t timestamp;      // 24-bit sensor counter (wraps at 0xFFFFFF)
    bool     valid = false;
};

// ---- Driver -----------------------------------------------------------------

class QMI8658A {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x6A;
    static constexpr uint8_t ALT_ADDR     = 0x6B;
    static constexpr uint8_t CHIP_ID      = 0x05;
    static constexpr float   DEG_TO_RAD   = 3.14159265358979f / 180.0f;
    static constexpr float   G_TO_MS2     = 9.80665f;

    QMI8658A(HalInterface& hal, uint8_t addr = DEFAULT_ADDR)
        : hal_(hal), addr_(addr) {}

    bool init(AccelScale aScale, AccelODR aOdr,
              GyroScale  gScale, GyroODR  gOdr,
              bool enableLpf = true,
              LPFMode lpfMode = LPFMode::BW_5_39) {

        uint8_t id = 0;
        if (!hal_.readRegister(addr_, QMI_REG::WHO_AM_I, &id) || id != CHIP_ID)
            return false;

        uint8_t ctrl2 = static_cast<uint8_t>(aScale) | static_cast<uint8_t>(aOdr);
        if (!hal_.writeRegister(addr_, QMI_REG::CTRL2, ctrl2)) return false;

        uint8_t ctrl3 = static_cast<uint8_t>(gScale) | static_cast<uint8_t>(gOdr);
        if (!hal_.writeRegister(addr_, QMI_REG::CTRL3, ctrl3)) return false;

        if (enableLpf) {
            uint8_t m = static_cast<uint8_t>(lpfMode);
            uint8_t ctrl5 = (m << 5) | (1 << 4) | (m << 1) | 1;
            if (!hal_.writeRegister(addr_, QMI_REG::CTRL5, ctrl5)) return false;
        }

        // SyncSample on, both sensors enabled
        if (!hal_.writeRegister(addr_, QMI_REG::CTRL7, 0x83)) return false;

        accelSens_ = accelSensitivity(aScale);
        gyroSens_  = gyroSensitivity(gScale);
        odr_       = odrHz(gOdr);
        return true;
    }

    bool dataReady() const {
        uint8_t s = 0;
        hal_.readRegister(addr_, QMI_REG::STATUSINT, &s);
        return (s & 0x01) != 0;
    }

    ImuSample read() const {
        ImuSample s;
        uint8_t ts[3], buf[14];

        if (!hal_.burstRead(addr_, QMI_REG::TIMESTAMP_L, ts, 3)) return s;
        if (!hal_.burstRead(addr_, QMI_REG::TEMP_L, buf, 14))    return s;

        s.temperature = static_cast<float>(static_cast<int8_t>(buf[1]))
                      + static_cast<float>(buf[0]) / 256.0f;

        auto to16 = [](uint8_t lo, uint8_t hi) -> int16_t {
            return static_cast<int16_t>(static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8));
        };

        float aInv = G_TO_MS2 / accelSens_;
        s.accel.x = to16(buf[2],  buf[3])  * aInv;
        s.accel.y = to16(buf[4],  buf[5])  * aInv;
        s.accel.z = to16(buf[6],  buf[7])  * aInv;

        float gInv = DEG_TO_RAD / gyroSens_;
        s.gyro.x = to16(buf[8],  buf[9])  * gInv;
        s.gyro.y = to16(buf[10], buf[11]) * gInv;
        s.gyro.z = to16(buf[12], buf[13]) * gInv;

        s.timestamp = static_cast<uint32_t>(ts[0])
                    | (static_cast<uint32_t>(ts[1]) << 8)
                    | (static_cast<uint32_t>(ts[2]) << 16);
        s.valid = true;
        return s;
    }

    float odrHz() const { return odr_; }

    bool runCalibrationOnDemand() {
        uint8_t ctrl7 = 0;
        if (!hal_.readRegister(addr_, QMI_REG::CTRL7, &ctrl7)) return false;
        if (!hal_.writeRegister(addr_, QMI_REG::CTRL7, ctrl7 & ~0x02u)) return false;
        if (!hal_.writeRegister(addr_, QMI_REG::CTRL9, 0xA2)) return false;
        // Caller should poll COD_STATUS (0x46) until done, then re-enable gyro
        return true;
    }

    bool reEnableGyro() {
        uint8_t ctrl7 = 0;
        if (!hal_.readRegister(addr_, QMI_REG::CTRL7, &ctrl7)) return false;
        return hal_.writeRegister(addr_, QMI_REG::CTRL7, ctrl7 | 0x02u);
    }

private:
    HalInterface& hal_;
    uint8_t addr_;
    float accelSens_ = 4096.0f;
    float gyroSens_  = 16.0f;
    float odr_       = 448.4f;

    static float accelSensitivity(AccelScale s) {
        switch (s) {
            case AccelScale::G2:  return 16384.0f;
            case AccelScale::G4:  return 8192.0f;
            case AccelScale::G8:  return 4096.0f;
            case AccelScale::G16: return 2048.0f;
        }
        return 4096.0f;
    }

    static float gyroSensitivity(GyroScale s) {
        switch (s) {
            case GyroScale::DPS16:   return 2048.0f;
            case GyroScale::DPS32:   return 1024.0f;
            case GyroScale::DPS64:   return 512.0f;
            case GyroScale::DPS128:  return 256.0f;
            case GyroScale::DPS256:  return 128.0f;
            case GyroScale::DPS512:  return 64.0f;
            case GyroScale::DPS1024: return 32.0f;
            case GyroScale::DPS2048: return 16.0f;
        }
        return 16.0f;
    }

    static float odrHz(GyroODR odr) {
        switch (odr) {
            case GyroODR::Hz7174: return 7174.4f;
            case GyroODR::Hz3587: return 3587.2f;
            case GyroODR::Hz1794: return 1793.6f;
            case GyroODR::Hz897:  return 896.8f;
            case GyroODR::Hz448:  return 448.4f;
            case GyroODR::Hz224:  return 224.2f;
            case GyroODR::Hz112:  return 112.1f;
            case GyroODR::Hz56:   return 56.05f;
            case GyroODR::Hz28:   return 28.025f;
        }
        return 448.4f;
    }
};

} // namespace sf
