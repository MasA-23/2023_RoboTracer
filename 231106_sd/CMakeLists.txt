#Set the mimum required version of CMake
cmake_minimum_required(VERSION 3.12)

set(outputname "main")

#Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

#Set the project name, the programming language
project(${outputname} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#Initialize the SDK
pico_sdk_init()

#Add an executable target to be build from the source files
add_executable(${outputname} ${outputname}.c
    ff.c
    ffsystem.c
    ffunicode.c
    mmc_pico_spi.c)

#Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(${outputname} pico_stdlib hardware_i2c hardware_pwm hardware_adc pico_multicore hardware_spi )

#Enable USB output, disable UART output
pico_enable_stdio_usb(${outputname} 1)
pico_enable_stdio_uart(${outputname} 0)

#Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${outputname})
