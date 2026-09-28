/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef TELLTALES_MODEL_H
#define TELLTALES_MODEL_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QTimer>
#include <QtEnvironmentVariables>

#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QtQmlIntegration/qqmlintegration.h>

#include <random>
#include <cmath>

class TellTalesModel: public QObject
{
    Q_OBJECT

    /** turnLeft */
    Q_PROPERTY(bool turnLeftActive MEMBER turnLeftActive NOTIFY turnLightChanged)
    Q_PROPERTY(bool turnLeftBlinking MEMBER turnLeftBlinking NOTIFY turnLightChanged)
    /** phone */
    Q_PROPERTY(bool phoneActive MEMBER phoneActive NOTIFY phoneActiveChanged)
    /** bluetooth */
    Q_PROPERTY(bool bluetoothActive MEMBER bluetoothActive NOTIFY bluetoothActiveChanged)
    /** findMy */
    Q_PROPERTY(bool findMyActive MEMBER findMyActive NOTIFY findMyActiveChanged)
    /** key */
    Q_PROPERTY(bool keyActive MEMBER keyActive NOTIFY keyActiveChanged)
    /** beam */
    Q_PROPERTY(bool beamActive MEMBER beamActive NOTIFY beamActiveChanged)
    /** highBeams */
    Q_PROPERTY(bool highBeamsActive MEMBER highBeamsActive NOTIFY highBeamsActiveChanged)
    /** parked */
    Q_PROPERTY(bool parkedActive MEMBER parkedActive NOTIFY parkedActiveChanged)
    /** helmet */
    Q_PROPERTY(bool helmetActive0 MEMBER helmetActive0 NOTIFY helmetActiveChanged)
    Q_PROPERTY(bool helmetActive1 MEMBER helmetActive1 NOTIFY helmetActiveChanged)
    /** turnRight */
    Q_PROPERTY(bool turnRightActive MEMBER turnRightActive NOTIFY turnLightChanged)
    Q_PROPERTY(bool turnRightBlinking MEMBER turnRightBlinking NOTIFY turnLightChanged)
    /** gpuPresent */
    Q_PROPERTY(bool gpuPresent MEMBER gpuPresent NOTIFY gpuPresentChanged)

    /** showGauge */
    Q_PROPERTY(bool showGauge MEMBER showGauge NOTIFY showGaugeChanged)

    /** Other proerpties */
    Q_PROPERTY(bool opacityChangeDuration MEMBER opacityChangeDuration CONSTANT)
    Q_PROPERTY(double qtLogoOpacity MEMBER qtLogoOpacity NOTIFY qtLogoOpacityChanged)
    Q_PROPERTY(double indicatorOpacity MEMBER indicatorOpacity NOTIFY indicatorOpacityChanged)

    /** Settings */
    Q_PROPERTY(bool settingsAapEnable MEMBER settings_aap_enable NOTIFY settingsChanged)
    Q_PROPERTY(bool settingsLlmEnable MEMBER settings_llm_enable NOTIFY settingsChanged)
    Q_PROPERTY(bool settingsKeyEnable MEMBER settings_key_enable NOTIFY settingsChanged)
    Q_PROPERTY(bool settingsFindMyEnable MEMBER settings_find_my_enable NOTIFY settingsChanged)
    Q_PROPERTY(bool settingsImuEnable MEMBER settings_imu_enable NOTIFY settingsChanged)
    Q_PROPERTY(bool settingsImuChaseView MEMBER settings_imu_chase_view NOTIFY settingsChanged)

    QML_ELEMENT
    QML_SINGLETON

public:
    static TellTalesModel* instance();
    ~TellTalesModel(void);

    Q_INVOKABLE void checkGpuPresent(void);

    bool settingsAapEnable(void);
    bool settingsLlmEnable(void);
    bool settingsKeyEnable(void);
    bool settingsFindMyEnable(void);
    bool settingsImuEnable(void);
    bool settingsImuChaseView(void);

Q_SIGNALS:
    void turnLightChanged();
    void phoneActiveChanged();
    void bluetoothActiveChanged();
    void findMyActiveChanged();
    void keyActiveChanged();
    void beamActiveChanged();
    void highBeamsActiveChanged();
    void parkedActiveChanged();
    void helmetActiveChanged();
    void gpuPresentChanged();
    void showGaugeChanged();

    void qtLogoOpacityChanged();
    void indicatorOpacityChanged();

    void settingsChanged();

public Q_SLOTS:
    void onPhoneDeviceConnectionChanged(bool b);
    void onHeadsetDeviceConnectionChanged(bool b0, bool b1);

private:
    explicit TellTalesModel(QObject* parent = nullptr);

    bool turnLeftActive;
    bool turnLeftBlinking;
    bool phoneActive;
    bool bluetoothActive;
    bool findMyActive;
    bool keyActive;
    bool beamActive;
    bool highBeamsActive;
    bool parkedActive;
    bool helmetActive0;
    bool helmetActive1;
    bool turnRightActive;
    bool turnRightBlinking;
    bool gpuPresent;
    bool showGauge;

    const int opacityChangeDuration = 500;
    double qtLogoOpacity;
    double indicatorOpacity;

    bool settings_aap_enable;
    bool settings_llm_enable;
    bool settings_key_enable;
    bool settings_find_my_enable;
    bool settings_imu_enable;
    bool settings_imu_chase_view;

    int turn_state; // -1: turn left; 0: straight; +1: turn right;
    QTimer turn_timer;

    void setGpuPresent(bool b);

private Q_SLOTS:
    void random_turn_light(void);
};

#endif // TELLTALES_MODEL_H
