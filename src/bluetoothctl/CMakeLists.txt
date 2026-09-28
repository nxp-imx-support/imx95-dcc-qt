
# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause
set(BT_MODULE  bluetoothctl)

set(BLUEZ_INCLUDE_DIRS  ${CMAKE_SYSROOT}/usr/include)
add_definitions(-DHAVE_CONFIG_H)

# add libs path
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez>5.44)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(DBUS REQUIRED dbus-1)

include_directories(
    client
    bluealsa/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BLUEZ_SRC_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${BLUEZ_INCLUDE_DIRS}
)

file(GLOB CPP_FILES *.cpp)

file(GLOB RFCOMM_FILES
    bluealsa/rfcomm.c
    bluealsa/src/shared/*.c
)

add_library(${BT_MODULE} STATIC ${CPP_FILES} ${RFCOMM_FILES})

target_link_libraries(${BT_MODULE} ${GLIB_LIBRARIES} ${DBUS_LIBRARIES} readline)
