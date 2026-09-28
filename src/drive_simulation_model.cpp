/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "drive_simulation_model.h"

DriveSimModel::DriveSimModel(QObject* parent) : QObject(parent)
{
    init();

    trigger.setSingleShot(false);
    trigger.setInterval(DriveSimModel::simulation_interval);
    connect(&trigger, &QTimer::timeout, [=](void) {
        drive();
        simulate();
        emit resultUpdated();
    });

    startSimulation();
}

DriveSimModel::~DriveSimModel()
{

}

DriveSimModel* DriveSimModel::instance()
{
    static std::unique_ptr<DriveSimModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new DriveSimModel);
    }
    return s_instance.get();
}

void DriveSimModel::startSimulation(void)
{
    init();
    trigger.start();
}
void DriveSimModel::stopSimulation(void)
{
    trigger.stop();
}


double DriveSimModel::throttle(void)
{
    return throttle_actual;
}

void DriveSimModel::setThrottle(double value)
{
    throttle_input.pop_front();
    throttle_input.push_back(value);
}

void DriveSimModel::init(void)
{
    random_timer = 8;
    direction_timer = 0;
    target_accel = 0.0;

    accel_error = 0.0;
    accel_error_intg = 0.0;

    accel_kp = 0.00625, accel_ki = 0.0001875;

    throttle_input.fill(0.0, 4);
    throttle_actual = 0.0;
    gear_level = 0;
    power_outage_counter = 0;
    bike_speed_f = 0.0;
    engine_rpm_f = 0.0;
    bike_speed = 0;
    engine_rpm = 3150;

    fuel_level = 10000.0;
    batt_level = 50.0;
    range_estimate = 500.0;
    odo_distance = 0.0;
}

void DriveSimModel::drive(void)
{
    random_timer--;
    if (random_timer == 0) {
        if (bike_speed_f < (15.0 / 3.5 * 1000)) {
            target_accel = (rand() % 10001) * (random_accle_h - (random_accle_l / 2.0)) / 10000.0 + (random_accle_l / 2.0);
        } else if (bike_speed_f > (105.0 / 3.5 * 1000)) {
            target_accel = (rand() % 10001) * ((random_accle_h / 2) - random_accle_l) / 10000.0 + random_accle_l;
        } else {
            target_accel = (rand() % 10001) * (random_accle_h - random_accle_l) / 10000.0 + random_accle_l;
        }
        if (target_accel > 0.0) {
            random_timer = 1U << (rand() % 5 + 3);
        } else {
            random_timer = 1U << (rand() % 2 + 2);
        }
        if (target_accel * accel_error_intg < 0.0) {
            direction_timer = 0;
            accel_error_intg = 0.0;
        } else {
            direction_timer++;
        }
        // qDebug() << "drive sim accel" << target_accel << "duration" << random_timer * 128 << "ms";
    }

    accel_error = (target_accel - final_accel);
    accel_error_intg += accel_error;

    double throttle_setting = accel_error * accel_kp + accel_error_intg * accel_ki;
    throttle_setting =  throttle_setting > 1.0 ? 1.0 : throttle_setting;
    throttle_setting =  throttle_setting < -1.0 ? -1.0 : throttle_setting; // negative throttle_actual represents brake, does not consume fuel

    setThrottle(throttle_setting);

    if (fuel_level < 1000.0) {
        fuel_level = 10000.0;
        range_estimate = 500.0;
    }

    if (gear_level > 0 && engine_rpm_f < 5250.0) {
        -- gear_level;
        power_outage_counter = 1.25 * simulation_freq;
    } else if (gear_level < (gear_num -1) && engine_rpm_f > 8400.0) {
        ++ gear_level;
        power_outage_counter = 0.75 * simulation_freq;
    }
}

void DriveSimModel::simulate(void)
{
    throttle_actual = 0.0;
    for (auto i : throttle_input) {
        throttle_actual += i;
    }
    throttle_actual = throttle_actual / throttle_input.size();

    double bike_speed_km_h_f = bike_speed_f / 1000.0 * 3.6;

    // calculate accel
    double accel;
    if (power_outage_counter > 0) {
        accel = 0.0;
        --power_outage_counter;
    } else {
        accel = -0.0001 * (engine_rpm_f + 2000) * (engine_rpm_f - 16000) * throttle_actual / simulation_freq;
        accel = accel * (rand() % 11 + 95) / 100;
    }

    // calculate drag
    double drag = (48 + 1.6 * bike_speed_km_h_f + 0.6 * bike_speed_km_h_f * bike_speed_km_h_f) / simulation_freq;
    // randomize by 15%
    drag = drag * (rand() % 31 + 85) / 100;
    drag = drag > bike_speed_f ? bike_speed_f : drag;

    final_accel = accel - drag;

    bike_speed_f = bike_speed_f + final_accel;
    bike_speed_f = bike_speed_f < 0.0 ? 0.0 : bike_speed_f;
    engine_rpm_f = bike_speed_km_h_f * gear_ratio[gear_level];

    bike_speed = bike_speed_km_h_f; // get km/h
    engine_rpm = engine_rpm_f;

    // calculate odo
    odo_distance += bike_speed / (3600 * simulation_freq);

    // estimate range
    // TODO
    range_estimate -= bike_speed / (3600 * simulation_freq);

    // calculate fuel
    if (throttle_actual > 0.0) {
        fuel_level -= fuel_consumption * throttle_actual / 3600.0 / simulation_freq;
    }
    fuel_level = fuel_level < 0.0 ? 0.0 : fuel_level;

    // calculate batt
    batt_level += (engine_rpm - 6000) / 90000.0 / simulation_freq;
    batt_level =  batt_level > batt_capacity ? batt_capacity : batt_level;
    batt_level =  batt_level < 0.0 ? 0.0 : batt_level;

    emit resultUpdated();
}
