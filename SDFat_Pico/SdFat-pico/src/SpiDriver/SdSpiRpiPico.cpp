 
 
 #include "SdSpiRpiPico.h"
 #if RPI_PICO

SdSpiRpiPico::SdSpiRpiPico(spi_inst_t* spi)
: _spi(spi)
{

}

void SdSpiRpiPico::activate(){
    // activate hardware
    spi_init(_spi,1000000);
}


// Initialize the SPI bus.
void SdSpiRpiPico::begin(SdSpiConfig config){
    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

// Deactivate SPI hardware. 
void SdSpiRpiPico::deactivate(){
    spi_deinit(_spi);
}

// Receive a byte.
uint8_t SdSpiRpiPico::receive(){
    uint8_t buff;
    spi_read_blocking(_spi, 0xFF, &buff, 1);
    return buff;
}
  
// Receive multiple bytes.
uint8_t SdSpiRpiPico::receive(uint8_t* buf, size_t count){
    int read = spi_read_blocking(_spi, 0xFF, buf, count );
    return (read == count) ? 0 : 1;  // no error if read correct number of bytes
}

// Send a byte.
void SdSpiRpiPico::send(uint8_t data){
    spi_write_blocking(_spi, &data, 1);
}

// Send multiple bytes.
void SdSpiRpiPico::send(const uint8_t* buf, size_t count){
    spi_write_blocking(_spi, buf, count);
}

// Save high speed SPISettings after SD initialization.
void SdSpiRpiPico::setSckSpeed(uint32_t maxSck){
    spi_set_baudrate(_spi, maxSck);
}

#endif // RPI_PICO