/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "telltales_model.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_tt, "model.telltales")

TellTalesModel::TellTalesModel(QObject* parent) : QObject(parent)
{
    turnLeftActive = false;
    turnLeftBlinking = false;
    phoneActive = false;
    bluetoothActive = true;
    findMyActive = true;
    keyActive = true;
    beamActive = true;
    highBeamsActive = true;
    parkedActive = true;
    helmetActive0 = false;
    helmetActive1 = false;
    turnRightActive = false;
    turnRightBlinking = false;
    gpuPresent = true;
    showGauge = true;

    qtLogoOpacity = 1;
    indicatorOpacity = 1;

    settings_aap_enable = false;
    settings_llm_enable = false;
    settings_key_enable = false;
    settings_find_my_enable = false;
    settings_imu_enable = false;
    settings_imu_chase_view = false;

    turn_state = 0;
    QObject::connect(&turn_timer, &QTimer::timeout, this, &TellTalesModel::random_turn_light);
    turn_timer.setSingleShot(true);
    turn_timer.start(5000);

    checkGpuPresent();

    /** detect window ratio */
    auto&& window_geo = QGuiApplication::primaryScreen()->availableGeometry();
    float window_width = window_geo.width();
    float window_height = window_geo.height();
    //qCDebug(lc_tt) << "window size" << window_width << window_height;

    if (window_width / 720 > 2.0f) {
        showGauge = true;
    } else {
        showGauge = false;
    }
    emit showGaugeChanged();

    bool isConfPresent = QFile{"/etc/dcc/app.conf"}.exists();
    QSettings settings("/etc/dcc/app.conf", QSettings::IniFormat);

    if (!isConfPresent) {
        qCDebug(lc_tt) << "config file /etc/dcc/app.conf is missing, create default";
        settings.setValue("AAP/enabled", settings_aap_enable);
        settings.setValue("LLM/enabled", settings_llm_enable);
        settings.setValue("Key/enabled", settings_key_enable);
        settings.setValue("FindMy/enabled", settings_find_my_enable);
        settings.setValue("FindMy/gpioChip", "/dev/gpiochip0");
        settings.setValue("FindMy/gpioPin", -1);
        settings.setValue("IMU/enabled", settings_imu_enable);
        settings.setValue("IMU/chaseView", settings_imu_chase_view);
    }

    settings_aap_enable = settings.value("AAP/enabled").toBool();
    settings_llm_enable = settings.value("LLM/enabled").toBool();
    settings_key_enable = settings.value("Key/enabled").toBool();
    settings_find_my_enable = settings.value("FindMy/enabled").toBool();
    settings_imu_enable = settings.value("IMU/enabled").toBool();
    settings_imu_chase_view = settings.value("IMU/chaseView").toBool();

    qCDebug(lc_tt) << "setting status" << settings.status();
    emit settingsChanged();
}

TellTalesModel* TellTalesModel::instance()
{
    static TellTalesModel* s_instance = nullptr;
    if (!s_instance)
        s_instance = new TellTalesModel();
    return s_instance;
}

TellTalesModel::~TellTalesModel()
{

}

bool TellTalesModel::settingsAapEnable(void)
{
    return settings_aap_enable;
}

bool TellTalesModel::settingsLlmEnable(void)
{
    return settings_llm_enable;
}

bool TellTalesModel::settingsKeyEnable(void)
{
    return settings_key_enable;
}

bool TellTalesModel::settingsFindMyEnable(void)
{
    return settings_find_my_enable;
}

bool TellTalesModel::settingsImuEnable(void)
{
    return settings_imu_enable;
}

bool TellTalesModel::settingsImuChaseView(void)
{
    return settings_imu_chase_view;
}

void TellTalesModel::checkGpuPresent(void)
{
    const char* env_name = "QMLSCENE_DEVICE";
    if (qEnvironmentVariableIsSet(env_name) &&
        qEnvironmentVariable(env_name) == QString("softwarecontext")) {
        qCDebug(lc_tt) << "No GPU mode detected";
        setGpuPresent(false);
        return;
    }
    setGpuPresent(true);
}

void TellTalesModel::random_turn_light(void)
{
    int turn_duration;
    if (turn_state == 0) {
        turn_state = ((int)rand()) % 3 - 1;
    } else {
        turn_state = 0;
    }
    if (turn_state == 0) {
        turn_duration = 5000 + ((int)rand()) % 15001; // 5 ~ 20s
        turnLeftActive = false;
        turnLeftBlinking = false;
        turnRightActive = false;
        turnRightBlinking = false;
    } else {
        turn_duration = 5000 + ((int)rand()) % 5001; // 5 ~ 10s
        if (turn_state == -1) {
            turnLeftActive = true;
            turnLeftBlinking = true;
            turnRightActive = false;
            turnRightBlinking = false;
        } else {
            turnLeftActive = false;
            turnLeftBlinking = false;
            turnRightActive = true;
            turnRightBlinking = true;
        }
    }
    //qCDebug(lc_tt) << "turn light state: " << turn_state << ", duration: " << turn_duration;
    emit turnLightChanged();
    turn_timer.start(turn_duration);
}

void TellTalesModel::onPhoneDeviceConnectionChanged(bool b)
{
    if (phoneActive != b) {
        phoneActive = b;
        qCDebug(lc_tt) << "telltale phone status" << phoneActive;
        emit phoneActiveChanged();
    }
}
void TellTalesModel::onHeadsetDeviceConnectionChanged(bool b0, bool b1)
{
    bool changed = false;
    if (helmetActive0 != b0) {
        helmetActive0 = b0;
        changed = true;
    }
    if (helmetActive1 != b1) {
        helmetActive1 = b1;
        changed = true;
    }
    if (changed) {
        qCDebug(lc_tt) << "telltale headset status" << helmetActive0 << helmetActive1;
        emit helmetActiveChanged();
    }
}

void TellTalesModel::setGpuPresent(bool b)
{
    if (gpuPresent == b){
        return;
    }
    gpuPresent = b;
    emit gpuPresentChanged();
}

