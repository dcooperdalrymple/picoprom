cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(NAME picoprom)
set(URL "https://github.com/dcooperdalrymple/picoprom")

project(${NAME} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(${NAME}
	${CMAKE_CURRENT_LIST_DIR}/src/configs.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/eeprom.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/picoprom.cpp
)

target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pico-xmodem)

pico_add_extra_outputs(${NAME})

target_link_libraries(${NAME} pico_stdlib pico_rand pico_xmodem)

pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)

option(VERIFY_ROM "Support rom verification with octal bus transceiver" OFF)
if(VERIFY_ROM)
	target_compile_definitions(${NAME} PUBLIC VERIFY_ROM=1)
endif()
