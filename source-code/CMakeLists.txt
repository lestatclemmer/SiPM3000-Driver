# update this to change where libusb is instaalled
set(LIBUSB_DIR "/usr/local/include/libusb-1.0/")

cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
project(sipm3k_driver)

set(EXC_OUT driver_main)
add_executable(${EXC_OUT} main.cc)

include_directories(
    "${PROJECT_SOURCE_DIR}/include"
)

add_subdirectory(src)
add_subdirectory(include)

target_sources(${EXC_OUT}
    PRIVATE
        main.cc
        main.hh
)

# usb headers and libraries
find_path(LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    PATH_SUFFIXES "include" "libusb" "libusb-1.0")
find_library(LIBUSB_LIBRARY
    NAMES usb-1.0)

target_link_libraries(
    ${EXC_OUT}
    PRIVATE ${LIBUSB_LIBRARY} dl
)

target_include_directories(${EXC_OUT}
    PRIVATE
        "${LIBUSB_DIR}"
)

file(COPY lib DESTINATION ${CMAKE_BINARY_DIR})

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
