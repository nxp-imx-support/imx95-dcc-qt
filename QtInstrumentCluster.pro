# Copyright 2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

QT += core quick multimedia serialport concurrent svg

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        src/bluetooth_device.cpp \
        src/findmy_loc_service.cpp \
        src/media_control_model.cpp \
        src/phone_call_model.cpp \
        src/drive_simulation_model.cpp \
        src/color_icon_provider.cpp \
        src/telltales_model.cpp \
        src/kill_me_model.cpp

RESOURCES += qml.qrc

TRANSLATIONS += \
    QtInstrumentCluster_en_GB.ts
CONFIG += lrelease
CONFIG += embed_translations

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    src/bluetooth_device.h \
    src/findmy_loc_service.h \
    src/media_control_model.h \
    src/phone_call_model.h \
    src/drive_simulation_model.h \
    src/color_icon_provider.h \
    src/telltales_model.h \
    src/kill_me_model.h

unix: CONFIG += link_pkgconfig
# TODO: only use libgpiod on aarch64 target
#contains(linux-g++:QMAKE_TARGET.arch, aarch64) {
unix: PKGCONFIG += libgpiod libgpiodcxx
#}
