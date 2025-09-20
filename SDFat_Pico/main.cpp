#include <stdio.h>
#include "pico/stdlib.h"

#define SS (-1)
#include "Print.h"
#include "SdFat-pico/src/SpiDriver/SdSpiRpiPico.h"
#include "SdFat-pico/src/SdFat.h"

const uint8_t LED_PIN = 25;

const uint8_t SCK_PIN = 18; // PIN 24, GPIO18 -> SCL
const uint8_t MOSI_PIN = 19; // PIN 25, GPIO19 -> SDA
const uint8_t MISO_PIN = 16; // PIN 21, GPIO16 -> DO
const uint8_t CS_PIN = 17;   //PIN 22, GPIO17 (SPIO-CS) -> CS



SdMillis_t SysCall::curTimeMS(){
    return time_us_64() / 1000 ;
}
  
void sdCsInit(SdCsPin_t pin) {
    gpio_set_function(pin, GPIO_FUNC_SPI);
}
 

void sdCsWrite(SdCsPin_t pin, bool level) {
    // NOP initially as SPI driving CS/SS directly
}


static bool endsWith(const char* str, const char*suffix){
  size_t len = strlen(str);
  size_t slen = strlen(suffix);

  const char* pos = str + len - slen;
  for(size_t i=0; i<slen; ++i) {
    if(pos[i] != suffix[i]){
      return false;
    }
  }
  return true;
}



extern "C" int main() {

    
    stdio_init_all();
    for(int i=0; i<20; ++i) {
        busy_wait_us_32(100000);
        printf(".");
    }
    printf("\n");
    printf("SD Test\n");
    


    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_set_function(CS_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);

    SdSpiRpiPico port(spi0);
    SdSpiConfig config(CS_PIN, 0, 2000000, &port);    

    SdFs sd;
    if(sd.begin(config)) {
        FsFile dir;
        FsFile file;

        int processedFile = 0;

        char name[256];
  
        // Open root directory
        if (dir.open("/")){
            // Open next file in root.
            // Warning, openNext starts at the current position of dir so a
            // rewind may be necessary in your application.
            while (file.openNext(&dir, O_RDONLY)) {
        
             if(!file.isHidden() && !file.isDir()){
                //file.printFileSize(&Serial);
                //file.printModifyDateTime(&Serial);
                //file.printName(&Serial);
                // Serial.println();
                file.getName(name, sizeof(name));
                if( endsWith(name, ".txt")) {
                    // process file
                     char line[512];  // in practice lines are up to about 350 chars.
                     while (file.fgets(line, sizeof(line)) > 0) {
                         line[sizeof(line)-1] = 0; // guarantee terminated
                         printf("%s\n",line);
                     }
                } 

                file.close();
            }
        }
        if (dir.getError()) {
            processedFile = 0; // indeterminate state so load defaults later
        }
        return processedFile;
        }

    }
}

