/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MOTION_MODEL
#define MOTION_MODEL

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <QVariant>
#include <QList>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QQueue>
#include <QQuaternion>
#include <QGenericMatrix>

#include <QDateTime>
#include <QDebug>
#include <QQuickImageProvider>

#include <QReadWriteLock>
#include <QtConcurrent>
#include <QPromise>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QProcess>

#include <iostream>
#include <memory>
#include <any>
#include <functional>

#include <iio.h>
#include <sys/epoll.h>

#include "telltales_model.h"

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>
#include "UKF/Config.h"
#include "UKF/Core.h"
#include "UKF/Integrator.h"
#include "UKF/MeasurementVector.h"
#include "UKF/StateVector.h"
#include "UKF/Types.h"


#define UKF_STATE_DIM 6
#define UKF_MEASUREMENT_DIM 6

/*
Parameters for sensor model.

Members are as follows:
    - accel_covariance: covariance of accelerometer readings (XYZ)
    - gyro_covariance: covariance of gyro readings (XYZ)
    - mag_covariance: covariance of magnetometer readings (XYZ)
*/
struct ukf_sensor_params_t {
    real_t accel_covariance[3];
    real_t gyro_covariance[3];
};

struct ukf_state_t {
    real_t attitude[4]; /* x, y, z, W */
    real_t angular_velocity[3]; /* rolling (rad/s), pitching (rad/s),
                                   yawing (rad/s) */
    real_t acceleration[3]; /* x, y, z (m/s^2) */
};

struct ukf_state_error_t {
    real_t attitude[3]; /* roll, pitch, yaw */
    real_t angular_velocity[3]; /* rolling (rad/s), pitching (rad/s),
                                   yawing (rad/s) */
};

struct ukf_sensor_errors_t {
    real_t accel_bias[3];
    real_t gyro_bias[3];
};

struct ukf_innovation_t {
    real_t accel[3];
    real_t gyro[3];
};

void ukf_init(void);

/*
Note: W, x, y, z in the parameters for ukf_set_attitude differs to the stored
attitude representation in struct ukf_state_t, which is x, y, z, W
*/
void ukf_set_attitude(real_t w, real_t x, real_t y, real_t z);
void ukf_set_angular_velocity(real_t x, real_t y, real_t z);

/* Functions for getting the state vector and covariance. */
void ukf_set_state(struct ukf_state_t *in);
void ukf_get_state(struct ukf_state_t *in);
void ukf_get_state_covariance(
    real_t state_covariance[UKF_STATE_DIM * UKF_STATE_DIM]);
void ukf_get_state_covariance_diagonal(
    real_t state_covariance_diagonal[UKF_STATE_DIM]);
void ukf_get_state_error(struct ukf_state_error_t *in);

void ukf_get_innovation(struct ukf_innovation_t *in);

/*
Functions for setting sensor data. Before each frame, call the sensor_clear()
function to clear old sensor data.
*/
void ukf_sensor_clear(void);
void ukf_sensor_set_accelerometer(real_t x, real_t y, real_t z);
void ukf_sensor_set_gyroscope(real_t x, real_t y, real_t z);

/*
UKF-related functions.
*/
void ukf_set_params(struct ukf_sensor_params_t *in);
void ukf_set_process_noise(real_t process_noise_covariance[UKF_STATE_DIM]);
/* dt is the time delta in seconds */
void ukf_iterate(float dt);

/*
Functions to get the current bias and scale factor estimates from the
parameter estimation filter.
*/
void ukf_get_parameters(struct ukf_sensor_errors_t *in);
void ukf_get_parameters_error(struct ukf_sensor_errors_t *in);

/*
Functions to access the compiled configuration
*/
enum ukf_precision_t {
    UKF_PRECISION_FLOAT = 0,
    UKF_PRECISION_DOUBLE = 1
};

uint32_t ukf_config_get_state_dim(void);
uint32_t ukf_config_get_measurement_dim(void);
enum ukf_precision_t ukf_config_get_precision(void);

class MotionRunner : public QThread
{
    Q_OBJECT

public:
    MotionRunner();
    ~MotionRunner();

    void run(void) override;

    bool getStopSignal(void);
    void setState(bool b);
    void sendStopSignal(void);
    QQuaternion getAttitude();

signals:
    void connectedChanged(bool b);
    void retryRequest(void);
    void messageReceived(QByteArray message);
    void attitudeChanged(QQuaternion q);

private:

    // base control
    QMutex stop_mutex;
    bool stop_signal;

    // iio device
    static constexpr char const *accl_dev_name = "icm42605-accel";
    static constexpr char const *accl_x_name = "accel_x";
    static constexpr char const *accl_y_name = "accel_y";
    static constexpr char const *accl_z_name = "accel_z";
    static constexpr char const *gyro_dev_name = "icm42605-gyro";
    static constexpr char const *gyro_x_name = "anglvel_x";
    static constexpr char const *gyro_y_name = "anglvel_y";
    static constexpr char const *gyro_z_name = "anglvel_z";
    static constexpr int epoll_event_max = 4;
    struct iio_buffer *accl_buffer, *gyro_buffer;

    QQuaternion attitude;

    /** imu to qt */
    QMatrix3x3 matrix_imu_to_qt_axis_swap;
    QMatrix3x3 matrix_imu_to_qt_axis_swap_tr;
    //QMatrix3x3 matrix_imu_to_qt_axis_mirror;
    //QMatrix3x3 matrix_imu_to_qt_prerotate;

    /** qt to 3d */
    //QMatrix3x3 matrix_qt_to_3d_axis_swap;
    //QMatrix3x3 matrix_qt_to_3d_axis_swap_tr;
    //QMatrix3x3 matrix_qt_to_3d_axis_mirror;
    QMatrix3x3 matrix_qt_to_3d_prerotate;
};


class MotionModel: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuaternion attitude READ getAttitude NOTIFY attitudeChanged)

    QML_ELEMENT
    QML_SINGLETON

public:

    ~MotionModel(void);
    static MotionModel* instance();

    QQuaternion getAttitude(void);

Q_SIGNALS:
    void attitudeChanged(QQuaternion q);

public Q_SLOTS:

private:

    explicit MotionModel(QObject* parent = nullptr);

    // motion
    MotionRunner runner;
};

#endif /** MOTION_MODEL */
