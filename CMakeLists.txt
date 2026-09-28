# Copyright 2024-2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(QtInstrumentCluster VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g3 -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(BLUEZ_SRC_DIRS  ${PROJECT_SOURCE_DIR}/src/bluetoothctl/bluez5/bluez-5.72)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS 3DAnimation 3DCore 3DExtras 3DInput 3DLogic 3DRender Gui Multimedia Quick SerialPort SerialBus Concurrent Svg)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libgpiod REQUIRED IMPORTED_TARGET libgpiod libgpiodcxx)
pkg_check_modules(libiio REQUIRED IMPORTED_TARGET libiio)
pkg_check_modules(eigen REQUIRED IMPORTED_TARGET eigen3)

add_subdirectory(src/egf-api/cpp)

qt_standard_project_setup()

qt_add_executable(QtInstrumentCluster WIN32 MACOSX_BUNDLE
    main.cpp
    src/bluetooth_device.cpp src/bluetooth_device.h
    src/media_control_model.cpp src/media_control_model.h
    src/phone_call_model.cpp src/phone_call_model.h
    src/drive_simulation_model.cpp src/drive_simulation_model.h
    src/telltales_model.cpp src/telltales_model.h
    src/color_icon_provider.cpp src/color_icon_provider.h
    src/kill_me_model.cpp src/kill_me_model.h
    src/generative_chat_model.cpp src/generative_chat_model.h
    src/dms.cpp src/dms.h
    src/motion_model.cpp src/motion_model.h
    src/DccCanBusHandler.cpp src/DccCanBusHandler.h
    src/DccFindMyModel.cpp src/DccFindMyModel.h
    src/DccKeyDistanceModel.cpp src/DccKeyDistanceModel.h
)

target_compile_definitions(QtInstrumentCluster PRIVATE
  UKF_DOUBLE_PRECISION=1
)

target_include_directories(QtInstrumentCluster PRIVATE
  src/ukf/include
)

target_link_libraries(QtInstrumentCluster PRIVATE
    Qt::3DAnimation
    Qt::3DCore
    Qt::3DExtras
    Qt::3DInput
    Qt::3DLogic
    Qt::3DRender
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Quick
    Qt::SerialPort
    Qt::SerialBus
    Qt::Concurrent
    Qt::Svg
    PkgConfig::libgpiod
    PkgConfig::libiio
    PkgConfig::eigen
    gstreamer-1.0
    g2d
    ${GLIB_LIBRARIES}
    gobject-2.0
    egf_api
)

# Resources:
set(qml_resource_files
    "images/gauges/gauge-frame.png"
    "images/gauges/highlight-normal.png"
    "images/gauges/needle-normal.png"
    "images/motor3d3.gif"
    "images/status/battery.png"
    "images/status/fuel.png"
    "images/telltales/phone_accept.png"
    "images/telltales/phone_deny.png"
    "images/top-line.png"
    "images/telltales/bluetooth.svg"
    "images/telltales/phone.svg"
    "images/telltales/turn_left.svg"
    "images/telltales/turn_right.svg"
    "images/telltales/findmy.svg"
    "images/telltales/front_light.svg"
    "images/telltales/key.svg"
    "images/telltales/helmet.svg"
    "images/telltales/user.svg"
    "images/telltales/robot.svg"
    "images/telltales/dms.svg"
    "images/media/pause.svg"
    "images/media/play.svg"
    "images/media/play-next.svg"
    "images/media/play-previous.svg"
    "images/media/compact-disc.svg"
    "images/media/android.svg"
    "images/media/link.svg"
    "images/media/mic.svg"
    "images/media/skull-crossbones.svg"
    "images/ebike/bg1.png"
    "images/ebike/ebike_gauge_needle.png"
    "images/ebike/gauge.png"
    "images/ebike/side.png"
    "images/NotoSansSC-Regular.ttf"
    "main.qml"
    "view/Style.qml"
    "view/BluetoothMediaController.qml"
    "view/BluetoothSelection.qml"
    "view/BluetoothSelectionItem.qml"
    "view/Gauge.qml"
    "view/LocatorBubble.qml"
    "view/MediaControlButton.qml"
    "view/NormalMode.qml"
    "view/PhoneCall.qml"
    "view/TellTales.qml"
    "view/TellTalesIndicator.qml"
    "view/ColorIcon.qml"
    "view/ColorIcon_gpu.qml"
    "view/ColorIcon_nogpu.qml"
    "view/AndroidAutoController.qml"
    "view/HelmetInterconController.qml"
    "view/EbikeUI.qml"
    "view/KillMeSwitch.qml"
    "view/GenerativeChatItem.qml"
    "view/GenerativeChat.qml"
    "view/ChatController.qml"
)

qt_add_resources(QtInstrumentCluster "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS QtInstrumentCluster
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET QtInstrumentCluster
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

add_subdirectory(src/bluetoothctl)
add_subdirectory(view/3d/motor)


target_link_libraries(QtInstrumentCluster PRIVATE
    bluetoothctl
    # Generated_QtQuick3D_Nxp_motorcycle
    Generated_QtQuick3D_Nxp_motorcycleplugin
)
