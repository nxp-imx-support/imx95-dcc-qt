/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QLocale>
#include <QTranslator>

#include <QtQml/qqmlextensionplugin.h>

#include "src/drive_simulation_model.h"
#include "src/bluetooth_device.h"
#include "src/telltales_model.h"
#include "src/media_control_model.h"
#include "src/phone_call_model.h"
#include "src/color_icon_provider.h"
#include "src/kill_me_model.h"
#include "src/generative_chat_model.h"
#include "src/dms.h"
#include "src/motion_model.h"
#include "src/DccFindMyModel.h"
#include "src/DccKeyDistanceModel.h"

Q_IMPORT_QML_PLUGIN(Nxp_motorcyclePlugin)

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("NXP");
    QCoreApplication::setOrganizationDomain("nxp.com");
    QCoreApplication::setApplicationName("Digital Connected Cluster Demo");

    // seed rng
    srand(time(0));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QtInstrumentCluster_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    qmlRegisterType<BluetoothDeviceInfo>("BluetoothDeviceInfo", 1, 0, "BluetoothDeviceInfo");
    qmlRegisterType<GenerativeChatMessage>("GenerativeChatMessage", 1, 0, "GenerativeChatMessage");
    qmlRegisterType<ColorIconModel>("ColorIconModel", 1, 0, "ColorIconModel");

    qmlRegisterSingletonType<TellTalesModel>("TellTalesModel", 1, 0, "TellTalesModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return TellTalesModel::instance();
    });
    qmlRegisterSingletonType<DriveSimModel>("DriveSimulation", 1, 0, "DriveSimulation", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return DriveSimModel::instance();
    });
    qmlRegisterSingletonType<BluetoothDevice>("BluetoothDevice", 1, 0, "BluetoothDevice", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return BluetoothDevice::instance();
    });
    DccCanBusHandler canbus{"can0"};
    DccFindMyModel find_my{canbus};
    DccKeyDistanceModel key_dist{canbus};
    qmlRegisterSingletonType<DccFindMyModel>("DccFindMyModel", 1, 0, "DccFindMyModel", [&find_my](QQmlEngine*, QJSEngine*) -> QObject* {
        return &find_my;
    });
    qmlRegisterSingletonType<DccKeyDistanceModel>("DccKeyDistanceModel", 1, 0, "DccKeyDistanceModel", [&key_dist](QQmlEngine*, QJSEngine*) -> QObject* {
        return &key_dist;
    });
    qmlRegisterSingletonType<MediaControlModel>("MediaControlModel", 1, 0, "MediaControlModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return MediaControlModel::instance();
    });
    qmlRegisterSingletonType<PhoneCallModel>("PhoneCallModel", 1, 0, "PhoneCallModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return PhoneCallModel::instance();
    });
    qmlRegisterSingletonType<KillMeModel>("KillMeModel", 1, 0, "KillMeModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return KillMeModel::instance();
    });
    qmlRegisterSingletonType<ControlBoxPageModel>("ControlBoxPageModel", 1, 0, "ControlBoxPageModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return ControlBoxPageModel::instance();
    });
    qmlRegisterSingletonType<GenerativeChatModel>("GenerativeChatModel", 1, 0, "GenerativeChatModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return GenerativeChatModel::instance();
    });
    MotionModel::instance();
    qmlRegisterSingletonType<MotionModel>("MotionModel", 1, 0, "MotionModel", [](QQmlEngine*, QJSEngine*) -> QObject* {
        return MotionModel::instance();
    });

    qmlRegisterSingletonType(QUrl("qrc:///view/Style.qml"), "Style", 1, 0, "Style");

    QObject::connect(BluetoothDevice::instance(), &BluetoothDevice::phoneDeviceConnectionChanged,
            TellTalesModel::instance(), &TellTalesModel::onPhoneDeviceConnectionChanged);
    QObject::connect(BluetoothDevice::instance(), &BluetoothDevice::headsetDeviceConnectionChanged,
            TellTalesModel::instance(), &TellTalesModel::onHeadsetDeviceConnectionChanged);

    QQmlApplicationEngine engine;
    engine.addImageProvider("media_cover_art", MediaControlModel::instance()->getCoverArtProvider());
    engine.addImageProvider("color_icon_provider", ColorIconProvider::instance());
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.addImportPath(":/");
    engine.load(url);

    return app.exec();
}
