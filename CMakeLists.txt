cmake_minimum_required(VERSION 3.13)
# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
project(faultier)
 
pico_sdk_init()

add_executable(faultier)

# add_subdirectory(nanopb)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/extra)
find_package(Nanopb REQUIRED)


pico_generate_pio_header(faultier ${CMAKE_CURRENT_LIST_DIR}/faultier/pio/spi.pio)
pico_generate_pio_header(faultier ${CMAKE_CURRENT_LIST_DIR}/faultier/pio/i2c.pio)
pico_generate_pio_header(faultier ${CMAKE_CURRENT_LIST_DIR}/faultier/pio/uart_rx.pio)
pico_generate_pio_header(faultier ${CMAKE_CURRENT_LIST_DIR}/faultier/pio/uart_tx.pio)
pico_generate_pio_header(faultier ${CMAKE_CURRENT_LIST_DIR}/swd/probe.pio)

target_sources(faultier PRIVATE
        main.c

        # PIO components
        faultier/pio/pio_spi.c
        faultier/pio/pio_i2c.c

        # PDND source
        faultier/faultier.c
        faultier/pdnd.c
        faultier/pdnd_spi.c
        
        # Display support
        faultier/pdnd_display.c
        faultier/display/FreeSans5pt7b.c
        faultier/display/FreeSans9pt7b.c
        faultier/display/font_renderer.c
        faultier/display/ssd1306.c

        # Proto
        proto/faultier.pb.c

        bootup.c
        bootup2.c

        glitcher/ft_pio.c
        glitcher/power_cycler.c
        glitcher/trigger_compiler.c
        glitcher/delay_compiler.c
        glitcher/glitch_compiler.c

        protocol.c
        
        usb_descriptors.c

        swd/tamarin_probe.c
        swd/swdchecker.c
        )

# Required so the build finds tusb_config.h
target_include_directories(faultier PRIVATE ${CMAKE_CURRENT_LIST_DIR})
pico_set_binary_type(faultier copy_to_ram)

# pico_set_binary_type(faultier no_flash)
target_link_libraries(faultier PRIVATE
        pico_stdlib
        pico_multicore
        tinyusb_device
        tinyusb_board
        hardware_pio
        hardware_i2c
        hardware_adc
        hardware_dma
        nanopb
        )

pico_add_extra_outputs(faultier)

# enable usb output, disable uart output
# pico_enable_stdio_usb(faultier 0)
pico_enable_stdio_uart(faultier 0)
