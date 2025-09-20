#include "pico/stdlib.h"
#include "spi.h"


SPI::SPI(){
}

SPI::~SPI(){
}


// SPI helper functions using methods in derived classes.
uint8_t SPI::transfer(uint8_t data){
    write_read(&data, &data, 1);
    return data;
}

uint16_t SPI::transfer16(uint16_t data){
    write16_read16(&data, &data, 1);
    return data;
}

uint8_t SPI::read(uint8_t txData){
    uint8_t data;
    read(txData, &data,1);
    return data;
}

uint16_t SPI::read16(uint16_t txData){
    uint16_t data;
    read16(txData, &data,1);
    return data;
}

void SPI::write(uint8_t data){
    write(&data, 1);
}

void SPI::write16(uint16_t data){
    write16(&data,1);
}


//  Hardware SPI


HardwareSPI::HardwareSPI(spi_inst_t *spi, uint8_t miso, uint8_t sck, uint8_t mosi, uint baudrate)
: spi(spi)
, _ss(0xFF)
{
    spi_init (spi, baudrate);

    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
}

HardwareSPI::~HardwareSPI() {
    spi_deinit(spi);
}



bool HardwareSPI::busy() {
    // SPI: SSPSR Register
	// bit 4,  BSY,  PrimeCell SSP busy flag, RO: 0 SSP is idle. 1 SSP is 
	// currently transmitting and/or receiving a frame or the
	// transmit FIFO is not empty
	return (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS) != 0;
}


// Set this as a dedicated SPI bus that has control of its own SS (CS) pin.
void HardwareSPI::setDedicated(uint8_t ss){
    gpio_set_function(ss, GPIO_FUNC_SPI);
    _ss = ss;
}

bool HardwareSPI::isDedicated() const {
  return _ss != 0xFF;
}


void HardwareSPI::set_format (uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order){
    spi_set_format(spi, data_bits, cpol, cpha, order);
}

void HardwareSPI::set_slave (bool slave){
    spi_set_slave(spi, slave);
}

size_t HardwareSPI::is_writable (){
    return spi_is_writable(spi);
}

size_t HardwareSPI::is_readable (){
    return spi_is_readable(spi);
}

int HardwareSPI::write_read(const uint8_t *src, uint8_t *dst, size_t len){
    return spi_write_read_blocking(spi, src, dst, len);
}

int HardwareSPI::write(const uint8_t *src, size_t len){
    return spi_write_blocking(spi, src, len);
}

int HardwareSPI::read(uint8_t repeated_tx_data, uint8_t *dst, size_t len) {
    return spi_read_blocking(spi, repeated_tx_data, dst, len);
}

int HardwareSPI::write16_read16(const uint16_t *src, uint16_t *dst, size_t len){
    return spi_write16_read16_blocking(spi, src, dst, len);
}

int HardwareSPI::write16(const uint16_t *src, size_t len) {
    return spi_write16_blocking(spi, src, len);
}

int HardwareSPI::read16(uint16_t repeated_tx_data, uint16_t *dst, size_t len){
    return spi_read16_blocking(spi, repeated_tx_data,dst, len);
}


///////////////////////////////////////////////////////////////////////////////
// Software SPI


SoftwareSPI::SoftwareSPI(uint8_t miso, uint8_t sck, uint8_t mosi)
: _miso(miso)
, _mosi(mosi)
, _sck(sck)
, _ss(0xFF)
, _bits(8)
, _order(SPI_MSB_FIRST)
{
  gpio_init(_miso);  
  gpio_set_dir(_miso, GPIO_IN);
  
  gpio_init(_mosi);  
  gpio_set_dir(_mosi, GPIO_OUT);
  gpio_put(_mosi,0);

  gpio_init(_sck);  
  gpio_set_dir(_sck, GPIO_OUT);
  gpio_put(_sck, 0);
}

SoftwareSPI::~SoftwareSPI() {
   // NOP
}

// Set this as a dedicated SPI bus that has control of its own SS (CS) pin.
void SoftwareSPI::setDedicated(uint8_t ss){
  _ss = ss;
  gpio_init(_ss);  
  gpio_set_dir(_ss, GPIO_OUT);
  gpio_put(_ss,true);
}

bool SoftwareSPI::isDedicated() const {
  return _ss != 0xFF;
}

bool SoftwareSPI::busy() {
  return false;  // never busy if this can be called. (single thread)
}


void SoftwareSPI::set_format (uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order){
    _bits = data_bits;
    (void) cpol;
    (void) cpha;
   _order = order;
 }

void SoftwareSPI::set_slave (bool slave){
    assert(!slave);
}

size_t SoftwareSPI::is_writable (){
    return true;
}

size_t SoftwareSPI::is_readable (){
    return true;
}

int SoftwareSPI::write_read(const uint8_t *src, uint8_t *dst, size_t len){
    if(len == 0) return 0;
    
    for(size_t idx = 0; idx<len; ++idx){
        if(isDedicated()){
            gpio_put(_ss, 0);
        }

        uint8_t b = src[idx];
        uint8_t r = 0;

        if(_order == SPI_MSB_FIRST) {
            uint8_t mask = 0x80;
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask >>= 1;
            }
        } else {  // SPI_LSB_FIRST
            uint8_t mask = 0x01;
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask <<= 1;
            }
        }
	gpio_put(_sck,0);
        if(isDedicated()){
            gpio_put(_ss, 1);
        }
    }
    return len;
}

int SoftwareSPI::write(const uint8_t *src, size_t len){
    if(len == 0) return 0;
    
    for(size_t idx = 0; idx<len; ++idx){
        if(isDedicated()){
            gpio_put(_ss, 0);
        }

        uint8_t b = src[idx];

        if(_order == SPI_MSB_FIRST) {
            uint8_t mask = 0x80;
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                mask >>= 1;
                gpio_put(_sck, 1);
            }
        } else {  // SPI_LSB_FIRST
            uint8_t mask = 0x01;
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                mask <<= 1;
                gpio_put(_sck, 1);
            }
        }

        gpio_put(_sck,0);
        if(isDedicated()){
            gpio_put(_ss, 1);
        }
    }

    return len;
}

int SoftwareSPI::read(uint8_t repeated_tx_data, uint8_t *dst, size_t len) {
    if(len == 0) return 0;
    
    for(size_t idx = 0; idx<len; ++idx){
        if(isDedicated()){
            gpio_put(_ss, 0);
        }

        uint8_t b = repeated_tx_data;
        uint8_t r = 0;

        if(_order == SPI_MSB_FIRST) {
            uint8_t mask = 0x80;
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask >>= 1;
            }
        } else {  // SPI_LSB_FIRST
            uint8_t mask = 0x01;
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask <<= 1;
            }
        }
	gpio_put(_sck,0);
        if(isDedicated()){
            gpio_put(_ss, 1);
        }
    }
    return len;
}

int SoftwareSPI::write16_read16(const uint16_t *src, uint16_t *dst, size_t len){
    if(len == 0) return 0;
    

    for(size_t idx = 0; idx<len; ++idx){
        if(isDedicated()){
            gpio_put(_ss, 0);
        }

        uint16_t b = src[idx];
        uint16_t r = 0;

        if(_order == SPI_MSB_FIRST) {
            uint16_t mask = 1 << (_bits-1);
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask >>= 1;
            }
        } else {  // SPI_LSB_FIRST
            uint16_t mask = 0x0001;
            for(int i=0; i<_bits; ++i) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask <<= 1;
            }
        }

        dst[idx] = r;

        gpio_put(_sck,0);
        if(isDedicated()){
            gpio_put(_ss, 1);
        }
    }

    return len;
}

int SoftwareSPI::write16(const uint16_t *src, size_t len) {
    if(len == 0) return 0;
    

    for(size_t idx = 0; idx<len; ++idx){
        if(isDedicated()){
            gpio_put(_ss, false);
        }

        uint16_t b = src[idx];

        if(_order == SPI_MSB_FIRST) {
            uint16_t mask = 1 << (_bits-1);
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                mask >>= 1;
                gpio_put(_sck, 1);
            }
        } else {  // SPI_LSB_FIRST
            uint16_t mask = 0x0001;
            for(int i=0; i<_bits; ++i) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                mask <<= 1;
                gpio_put(_sck, 1);
            }
        }

        gpio_put(_sck,0);
        if(isDedicated()){
            gpio_put(_ss, 1);
        }
    }
    
    return len;
}

int SoftwareSPI::read16(uint16_t repeated_tx_data, uint16_t *dst, size_t len){
    if(len == 0) return 0;
    
    for(size_t idx = 0; idx<len; ++idx){
        if(isDedicated()){
            gpio_put(_ss, false);
        }

        uint16_t b = repeated_tx_data;
        uint16_t r = 0;

        if(_order == SPI_MSB_FIRST) {
            uint16_t mask = 1 << (_bits-1);
            while(mask) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask >>= 1;
            }
        } else {  // SPI_LSB_FIRST
            uint16_t mask = 0x0001;
            for(int i=0; i<_bits; ++i) {
                gpio_put(_sck,0);
                gpio_put(_mosi, (b & mask) ? 1 : 0);
                r |= gpio_get(_miso) ? mask : 0;
                gpio_put(_sck, 1);
                mask <<= 1;
            }
        }

        dst[idx] = r;

        gpio_put(_sck,0);
        if(isDedicated()){
            gpio_put(_ss, 1);
        }
    }
    
    return len;
}


