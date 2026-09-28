/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DRIVE_SIMULATION_MODEL
#define DRIVE_SIMULATION_MODEL

#include <QList>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QTime>

#include <QtQml/qqmlregistration.h>

#include <QDebug>

#include <iostream>
#include <memory>
#include <any>

class DriveSimModel: public QObject
{
    Q_OBJECT

    Q_PROPERTY(double bikeSpeed MEMBER bike_speed NOTIFY resultUpdated)
    Q_PROPERTY(int engineRpm MEMBER engine_rpm NOTIFY resultUpdated)
    Q_PROPERTY(int gearLevel MEMBER gear_level NOTIFY resultUpdated)
    Q_PROPERTY(double odoDistance MEMBER odo_distance NOTIFY resultUpdated)
    Q_PROPERTY(double rangeEstimate MEMBER range_estimate NOTIFY resultUpdated)
    Q_PROPERTY(double fuelLevel MEMBER fuel_level NOTIFY resultUpdated)
    Q_PROPERTY(double fuelCapacity READ fuelCapacity NOTIFY resultUpdated)
    Q_PROPERTY(double battLevel MEMBER batt_level NOTIFY resultUpdated)
    Q_PROPERTY(double battCapacity READ battCapacity NOTIFY resultUpdated)

    QML_ELEMENT
    QML_SINGLETON

public:
    static DriveSimModel* instance(void);
    ~DriveSimModel(void);

    Q_INVOKABLE void startSimulation(void);
    Q_INVOKABLE void stopSimulation(void);

    /* Getter group */
    double throttle(void);

Q_SIGNALS:
    void resultUpdated(void);

private:
    explicit DriveSimModel(QObject* parent = nullptr);

    QTimer trigger;

    /* Driver */
    static constexpr double random_accle_l = -625.0; // -40 m/s^2, mm/(1/8s)^2
    static constexpr double random_accle_h = 312.5; // 20 m/s^2, mm/(1/8s)^2

    uint32_t random_timer;
    uint32_t direction_timer;
    double target_accel;

    double accel_error, accel_error_intg;
    double accel_kp, accel_ki;

    /* Static parameters */
    static constexpr int simulation_freq = 8;
    static constexpr int simulation_interval = 125; // 125 ms
    static constexpr int gear_num = 6; // 6 Gears
    static constexpr double fuel_capacity = 12000; // 12 L
    static constexpr double fuel_consumption = 1600; // 1.6L / h
    static constexpr double batt_capacity = 60; // 60 Wh

    static constexpr double gear_ratio[6] = { // rpm = km/h * ratio
            207.29, 152.38, 121.79, 104.5, 91.96, 82.46,
        };

    /* Motor simulation */
    QList<double> throttle_input;
    double throttle_actual;
    void setThrottle(double value);

    inline double fuelCapacity(void) { return fuel_capacity; }
    inline double battCapacity(void) { return batt_capacity; }

    int gear_level;
    int power_outage_counter;

    /* Internal status */
    double final_accel;
    double bike_speed_f; // in mm/s
    double engine_rpm_f; // in rpm

    /* Output status */
    double bike_speed; // in km/h
    int engine_rpm; // in rpm

    double fuel_level;
    double batt_level;
    double range_estimate;
    double odo_distance;

    void init(void);
    void drive(void);
    void simulate(void);
};

#endif // DRIVE_SIMULATION_MODEL
