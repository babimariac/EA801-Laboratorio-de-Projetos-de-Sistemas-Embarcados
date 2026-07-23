# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Projeto1_Barbara_Glauco C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Adiciona os arquivos do projeto
add_executable(Projeto1_Barbara_Glauco
    Projeto1_Barbara_Glauco.c
    inc/ssd1306_i2c.c
)

# Nome e versão do programa
pico_set_program_name(Projeto1_Barbara_Glauco "Projeto1_Barbara_Glauco")
pico_set_program_version(Projeto1_Barbara_Glauco "0.1")

# Saída padrão: USB (UART desativado)
pico_enable_stdio_uart(Projeto1_Barbara_Glauco 0)
pico_enable_stdio_usb(Projeto1_Barbara_Glauco 1)

# Bibliotecas necessárias
target_link_libraries(Projeto1_Barbara_Glauco
    pico_stdlib
    hardware_adc
    hardware_i2c
    hardware_pwm
)

# Diretórios de includes (headers)
target_include_directories(Projeto1_Barbara_Glauco PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Geração de UF2, bin, etc.
pico_add_extra_outputs(Projeto1_Barbara_Glauco)
