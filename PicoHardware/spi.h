#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "hardware/spi.h"


// SPI abstract base class for both hardware and software SPI
// Use standard C++ virtual functions to allow mix of hardware
// and software SPI to co-exist rather than compile time fix.
// Overhead small enough to be fine on Pico etc.
class SPI {


public:
SPI();
virtual ~SPI();

// Helper functions that are closer to Arduino equivalents.
uint8_t transfer(uint8_t data);
uint16_t transfer16(uint16_t data);
uint8_t read(uint8_t txData = 0);
uint16_t read16(uint16_t txData = 0);
void write(uint8_t data);
void write16(uint16_t data);

// Set this as a dedicated SPI bus that has control of its own SS (CS) pin.
virtual void setDedicated(uint8_t ss) = 0;
virtual bool isDedicated() const = 0;

// Additional check to allow activity to be separated (e.g. to set other control bits)
// Needed for some graphics controllers that use seperate control/data pins
virtual bool busy() = 0;

// Direct equivalent for PICO SPI
virtual void set_format (uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order) = 0;
virtual void set_slave (bool slave) = 0;
virtual size_t is_writable () = 0;
virtual size_t is_readable () = 0;
virtual int write_read(const uint8_t *src, uint8_t *dst, size_t len) = 0;
virtual int write(const uint8_t *src, size_t len) = 0;
virtual int read(uint8_t repeated_tx_data, uint8_t *dst, size_t len) = 0;
virtual int write16_read16(const uint16_t *src, uint16_t *dst, size_t len) = 0;
virtual int write16(const uint16_t *src, size_t len) = 0;
virtual int read16(uint16_t repeated_tx_data, uint16_t *dst, size_t len) = 0;
};

class HardwareSPI : public SPI{

spi_inst_t * spi;
uint8_t _ss;   // slave select for dedicated SPI. 0xFF if not set

public:
HardwareSPI(spi_inst_t *spi, uint8_t miso, uint8_t sck, uint8_t mosi, uint baudrate);
~HardwareSPI();

// Set this as a dedicated SPI bus that has control of its own SS (CS) pin.
virtual void setDedicated(uint8_t ss);
virtual bool isDedicated() const;

// Additional check to allow activity to be separated (e.g. to set other control bits)
// Needed for some graphics controllers that use seperate control/data pins
virtual bool busy();

// Direct equivalent for PICO SPI
virtual void set_format (uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order);
virtual void set_slave (bool slave);
virtual size_t is_writable ();
virtual size_t is_readable ();
virtual int write_read(const uint8_t *src, uint8_t *dst, size_t len);
virtual int write(const uint8_t *src, size_t len);
virtual int read(uint8_t repeated_tx_data, uint8_t *dst, size_t len);
virtual int write16_read16(const uint16_t *src, uint16_t *dst, size_t len);
virtual int write16(const uint16_t *src, size_t len);
virtual int read16(uint16_t repeated_tx_data, uint16_t *dst, size_t len);
};



class SoftwareSPI : public SPI{

uint8_t _miso;
uint8_t _mosi;
uint8_t _sck;
uint8_t _ss;   
uint8_t _bits;
spi_order_t _order;


public:
SoftwareSPI(uint8_t miso, uint8_t sck, uint8_t mosi);
virtual ~SoftwareSPI();

// Set this as a dedicated SPI bus that has control of its own SS (CS) pin.
virtual void setDedicated(uint8_t ss);
virtual bool isDedicated() const;

// Additional check to allow activity to be separated (e.g. to set other control bits)
// Needed for some graphics controllers that use seperate control/data pins
virtual bool busy();

// Direct equivalent for PICO SPI
virtual void set_format (uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order);
virtual void set_slave (bool slave);
virtual size_t is_writable ();
virtual size_t is_readable ();
virtual int write_read(const uint8_t *src, uint8_t *dst, size_t len);
virtual int write(const uint8_t *src, size_t len);
virtual int read(uint8_t repeated_tx_data, uint8_t *dst, size_t len);
virtual int write16_read16(const uint16_t *src, uint16_t *dst, size_t len);
virtual int write16(const uint16_t *src, size_t len);
virtual int read16(uint16_t repeated_tx_data, uint16_t *dst, size_t len);
};


#endif
