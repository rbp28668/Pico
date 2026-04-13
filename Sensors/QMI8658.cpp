//#include "QMI8658.h"
#include "QMI8658.h"
#include <math.h>
#include <stdio.h>

QMI8658::QMI8658(I2C *pI2C, uint8_t addr) : pI2C(pI2C),
                                            addr(addr)
{

    uint8_t id = 0;

    uint8_t reg = (uint8_t)QMI8658_WHO_AM_I;
    pI2C->write(addr, &reg,1,true );
    pI2C->read(addr, &id, 1, false);
    if (0x05 != id)
    {
        printf("QMI8658 not found!\r\n");
        return;
    }

    reset();
}

void QMI8658::reset()
{
    write((uint8_t)QMI8658_RESET, 0xb0); // Software reset
    sleep_ms(15);                                           // Data sheet saays wait 15mS
    write((uint8_t)QMI8658_CTRL1,0x40); // CTRL1 Serial auto-increment, little endian, int 1 & 2 hi-z, high seed osc enabled.
    write((uint8_t)QMI8658_CTRL7,0x03); // CTRL7 Accelerometer and gyro enabled.
    write((uint8_t)QMI8658_CTRL2,0x95); // CTRL2 1001 1001 - accel self test + 4g,  250Hz Output data rate (ODRP)
    write((uint8_t)QMI8658_CTRL3,0xd5); // CTRL3 1101 1001 - gyro self test +  512dps,  250Hz ODR
    sleep_ms(100);
}

void QMI8658::read()
{
    uint8_t status;
    read(QMI8658_STATUS0, &status, 1);
    if (status & 0x03)
    {
        read((uint8_t)QMI8658_AX_L, (uint8_t *)data, 12);
    }
    else
        printf("QMI8658 read data fail!");
}

void QMI8658::angles(QMI8658::XYZ &xyz)
{
    float mask = accf_x() / sqrt((accf_y() * accf_y() + accf_z() * accf_z()));
    xyz.x = atan(mask) * 57.29578f; // 180/π=57.29578
    mask = accf_y() / sqrt((accf_x() * accf_x() + accf_z() * accf_z()));
    xyz.y = atan(mask) * 57.29578f; // 180/π=57.29578
    mask = sqrt((accf_x() * accf_x() + accf_y() * accf_y())) / accf_z();
    xyz.z = atan(mask) * 57.29578f; // 180/π=57.29578
}