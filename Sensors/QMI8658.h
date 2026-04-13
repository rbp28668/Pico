#ifndef QMI8658_H
#define QMI8658_H

#include "../PicoHardware/i2c.h"
#define QMI8658_PRIMARY_I2C 0x6A   // SDO at Vcc or float
#define QMI8658_SECONDARY_I2C 0x6B // SDO grounded

// Int 1 and int 2 ?
// Note Output Data Rate (ODR)

class QMI8658
{

    enum qmi8658_reg : int8_t
    {
        QMI8658_WHO_AM_I,
        QMI8658_REVISION_ID,
        QMI8658_CTRL1,
        QMI8658_CTRL2,
        QMI8658_CTRL3,
        QMI8658_CTRL4, // reserved
        QMI8658_CTRL5,
        QMI8658_CTRL6, // reserved
        QMI8658_CTRL7,
        QMI8658_CTRL8,
        QMI8658_CTRL9,
        QMI8658_CATL1_L,
        QMI8658_CATL1_H,
        QMI8658_CATL2_L,
        QMI8658_CATL2_H,
        QMI8658_CATL3_L,
        QMI8658_CATL3_H,
        QMI8658_CATL4_L,
        QMI8658_CATL4_H,
        QMI8658_FIFO_WTM_TH,
        QMI8658_FIFO_CTRL,
        QMI8658_FIFO_SMPL_CNT,
        QMI8658_FIFO_STATUS,
        QMI8658_FIFO_DATA,
        QMI8658_STATUSINT = 45,
        QMI8658_STATUS0,
        QMI8658_STATUS1,
        QMI8658_TIMESTAMP_LOW,
        QMI8658_TIMESTAMP_MID,
        QMI8658_TIMESTAMP_HIGH,
        QMI8658_TEMP_L,
        QMI8658_TEMP_H,
        QMI8658_AX_L,
        QMI8658_AX_H,
        QMI8658_AY_L,
        QMI8658_AY_H,
        QMI8658_AZ_L,
        QMI8658_AZ_H,
        QMI8658_GX_L,
        QMI8658_GX_H,
        QMI8658_GY_L,
        QMI8658_GY_H,
        QMI8658_GZ_L,
        QMI8658_GZ_H,
        QMI8658_COD_STATUS = 70,
        QMI8658_dQW_L = 73,
        QMI8658_dQW_H,
        QMI8658_dQX_L,
        QMI8658_dQX_H,
        QMI8658_dQY_L,
        QMI8658_dQY_H,
        QMI8658_dQZ_L,
        QMI8658_dQZ_H,
        QMI8658_dVX_L,
        QMI8658_dVX_H,
        QMI8658_dVY_L,
        QMI8658_dVY_H,
        QMI8658_dVZ_L,
        QMI8658_dVZ_H,
        QMI8658_TAP_STATUS = 89,
        QMI8658_STEP_CNT_LOW,
        QMI8658_STEP_CNT_MIDL,
        QMI8658_STEP_CNT_HIGH,
        QMI8658_RESET = 96
    };

private:
    I2C *pI2C;
    uint8_t addr;  // I2C addr

    uint16_t data[6];

    inline void write(uint8_t reg, uint8_t val){
        pI2C->write(addr, &reg, 1, true);
        pI2C->write(addr, &val, 1, false);
    }

    inline void read(uint8_t reg, uint8_t* data, size_t nbytes){
        pI2C->write(addr, &reg, 1, true);
        pI2C->read(addr, data, nbytes, false);
    }

public:

    struct XYZ{
        float x;
        float y;
        float z;
    };

    QMI8658(I2C *pI2C, uint8_t addr);
    void reset(); // allow 15mS
    void read();
    void angles(QMI8658::XYZ& xyz);

    inline uint16_t acc_x() { return data[0]; }
    inline uint16_t acc_y() { return data[1]; }
    inline uint16_t acc_z() { return data[2]; }
    inline uint16_t gyro_x() { return data[3]; }
    inline uint16_t gyro_y() { return data[4]; }
    inline uint16_t gyro_z() { return data[5]; }

    inline float accf_x() { return (float)data[0]; }
    inline float accf_y() { return (float)data[1]; }
    inline float accf_z() { return (float)data[2]; }
    inline float gyrof_x() { return (float)data[3]; }
    inline float gyrof_y() { return (float)data[4]; }
    inline float gyrof_z() { return (float)data[5]; }

};

#endif