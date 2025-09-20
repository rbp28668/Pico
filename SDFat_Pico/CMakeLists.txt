

cmake_minimum_required(VERSION 3.16)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)


project(PicoSDCard C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_definitions(RPI_PICO=1)

add_executable(PicoSDCard
        main.cpp
        spi.cpp
        SdFat-pico/src/MinimumSerial.cpp
        SdFat-pico/src/FreeStack.cpp
        SdFat-pico/src/common/FmtNumber.cpp
        SdFat-pico/src/common/FsCache.cpp
        SdFat-pico/src/common/FsDateTime.cpp
        SdFat-pico/src/common/FsStructs.cpp
        SdFat-pico/src/common/PrintBasic.cpp
        SdFat-pico/src/ExFatLib/ExFatDbg.cpp
        SdFat-pico/src/ExFatLib/ExFatFile.cpp
        SdFat-pico/src/ExFatLib/ExFatFilePrint.cpp
        SdFat-pico/src/ExFatLib/ExFatFileWrite.cpp
        SdFat-pico/src/ExFatLib/ExFatFormatter.cpp
        SdFat-pico/src/ExFatLib/ExFatPartition.cpp
        SdFat-pico/src/ExFatLib/ExFatVolume.cpp
        SdFat-pico/src/ExFatLib/upcase.cpp
        SdFat-pico/src/FatLib/FatDbg.cpp
        SdFat-pico/src/FatLib/FatFile.cpp
        SdFat-pico/src/FatLib/FatFileLFN.cpp
        SdFat-pico/src/FatLib/FatFilePrint.cpp
        SdFat-pico/src/FatLib/FatFileSFN.cpp
        SdFat-pico/src/FatLib/FatFormatter.cpp
        SdFat-pico/src/FatLib/FatPartition.cpp
        SdFat-pico/src/FatLib/FatVolume.cpp
        SdFat-pico/src/FsLib/FsFile.cpp
        SdFat-pico/src/FsLib/FsNew.cpp
        SdFat-pico/src/FsLib/FsVolume.cpp
        SdFat-pico/src/iostream/istream.cpp
        SdFat-pico/src/iostream/ostream.cpp
        SdFat-pico/src/iostream/StdioStream.cpp
        SdFat-pico/src/iostream/StreamBaseClass.cpp
        SdFat-pico/src/SdCard/SdCardInfo.cpp
        SdFat-pico/src/SdCard/SdSpiCard.cpp
        SdFat-pico/src/SpiDriver/SdSpiChipSelect.cpp
        SdFat-pico/src/SpiDriver/SdSpiRpiPico.cpp
        )


        
# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(PicoSDCard
        pico_stdlib 
         hardware_spi
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(PicoSDCard 1)
pico_enable_stdio_uart(PicoSDCard 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(PicoSDCard)

