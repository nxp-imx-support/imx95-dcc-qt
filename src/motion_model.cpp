/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "motion_model.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_motion, "model.motion")

/*
This is an implementation of an Unscented Kalman filter for a 9-axis AHRS,
using accelerometer and gyroscope to estimate attitude and angular velocity.
*/

/* Value of g in m/s^2. */
#define G_ACCEL (9.80665)

/* Value of Pi */
#define PI (3.1415925535)

enum AHRS_Keys {
    /* AHRS filter fields. */
    Attitude,
    AngularVelocity,

    /* Parameter estimation filter fields. */
    AccelerometerBias,
    GyroscopeBias,

    /* AHRS measurement vector fields. */
    Accelerometer,
    Gyroscope,
};

/*
The AHRS state vector contains the following:
- Attitude as a quaternion (NED frame to body frame)
- Angular velocity (body frame, rad/s)
*/
using AHRS_StateVector = UKF::StateVector<
    UKF::Field<Attitude, UKF::Quaternion>,
    UKF::Field<AngularVelocity, UKF::Vector<3>>
    >;

namespace UKF {
namespace Parameters {
template <> constexpr real_t AlphaSquared<AHRS_StateVector> = 1e-2;
template <> constexpr real_t Kappa<AHRS_StateVector> = 3.0;
}
}

/*
In addition to the AHRS filter, an online parameter estimation filter is also
implemented in order to calculate biases in each of the sensors.
The magnetometer scale factor is represented as a direction cosine matrix
with no normalisation constraint.
*/

using AHRS_SensorErrorVector = UKF::StateVector<
    UKF::Field<AccelerometerBias, UKF::Vector<3>>,
    UKF::Field<GyroscopeBias, UKF::Vector<3>>
    >;

namespace UKF {
namespace Parameters {
template <> constexpr real_t AlphaSquared<AHRS_SensorErrorVector> = 0.0001;
template <> constexpr real_t Beta<AHRS_SensorErrorVector> = 2.0;
template <> constexpr real_t Kappa<AHRS_SensorErrorVector> = 1.0;
}

/* AHRS process model. */
template <> template <>
AHRS_StateVector AHRS_StateVector::derivative<>() const {
    AHRS_StateVector output;

    /* Calculate change in attitude. */
    UKF::Quaternion omega_q;
    omega_q.vec() = get_field<AngularVelocity>() * 0.5;
    omega_q.w() = 0;
    output.set_field<Attitude>(omega_q.conjugate() * get_field<Attitude>());

    /* Assume constant angular velocity. */
    output.set_field<AngularVelocity>(UKF::Vector<3>(0, 0, 0));

    return output;
}
}

using AHRS_MeasurementVector = UKF::DynamicMeasurementVector<
    UKF::Field<Accelerometer, UKF::Vector<3>>,
    UKF::Field<Gyroscope, UKF::Vector<3>>
    >;

namespace UKF {
/*
This is the measurement model that's actually used in the filter, because
it's the one which takes the parameter estimation filter state as an input.
*/
template <> template <>
UKF::Vector<3> AHRS_MeasurementVector::expected_measurement
    <AHRS_StateVector, Accelerometer, AHRS_SensorErrorVector>(
        const AHRS_StateVector& state, const AHRS_SensorErrorVector& input) {
    return input.get_field<AccelerometerBias>() + state.get_field<Attitude>() * UKF::Vector<3>(0, 0, -G_ACCEL);
}

template <> template <>
UKF::Vector<3> AHRS_MeasurementVector::expected_measurement
    <AHRS_StateVector, Gyroscope, AHRS_SensorErrorVector>(
        const AHRS_StateVector& state, const AHRS_SensorErrorVector& input) {
    return input.get_field<GyroscopeBias>() + state.get_field<AngularVelocity>();
}

}

using AHRS_Filter = UKF::SquareRootCore<
    AHRS_StateVector,
    AHRS_MeasurementVector,
    UKF::IntegratorHeun
    >;

namespace UKF {
/*
AHRS parameter estimation filter process model. Since the evolution of sensor
errors is by definition unpredictable, this does nothing.
*/
template <> template <>
AHRS_SensorErrorVector AHRS_SensorErrorVector::derivative<>() const {
    return AHRS_SensorErrorVector::Zero();
}

/*
AHRS parameter estimation filter measurement model. These take in the current
state estimate and sensor scale factor and bias estimates, and use them to
calculate predicted measurements.
These functions are just the same as the state measurement model, but with
their arguments flipped.
*/
template <> template <>
UKF::Vector<3> AHRS_MeasurementVector::expected_measurement
    <AHRS_SensorErrorVector, Accelerometer, AHRS_StateVector>(
        const AHRS_SensorErrorVector& state, const AHRS_StateVector& input) {
    return state.get_field<AccelerometerBias>() + input.get_field<Attitude>() * UKF::Vector<3>(0, 0, -G_ACCEL);
}

template <> template <>
UKF::Vector<3> AHRS_MeasurementVector::expected_measurement
    <AHRS_SensorErrorVector, Gyroscope, AHRS_StateVector>(
        const AHRS_SensorErrorVector& state, const AHRS_StateVector& input) {
    return state.get_field<GyroscopeBias>() + input.get_field<AngularVelocity>();
}

}

/* Just use the Euler integrator since there's no process model. */
using AHRS_ParameterEstimationFilter = UKF::SquareRootParameterEstimationCore<
    AHRS_SensorErrorVector,
    AHRS_MeasurementVector
    >;

static AHRS_Filter ahrs;
static AHRS_ParameterEstimationFilter ahrs_errors;
static AHRS_MeasurementVector meas;
static UKF::Vector<3> acceleration;

/*
The following functions provide a ctypes-compatible interface for ease of
testing.
*/

void ukf_init() {
    /* Initialise state vector and covariance. */
    ahrs.state.set_field<Attitude>(UKF::Quaternion(1, 0, 0, 0));
    ahrs.state.set_field<AngularVelocity>(UKF::Vector<3>(0, 0, 0));
    acceleration << UKF::Vector<3>(0, 0, -G_ACCEL);
    ahrs.root_covariance = AHRS_StateVector::CovarianceMatrix::Zero();
    ahrs.root_covariance.diagonal() <<
        1e-1 * UKF::Vector<3>::Ones(),
        1e-3 * UKF::Vector<3>::Ones();

    /* Set measurement noise covariance. */
    ahrs.measurement_root_covariance <<
        0.3 * UKF::Vector<3>::Ones(),
        0.15 * UKF::Vector<3>::Ones();

    /* Set process noise covariance. */
    ahrs.process_noise_root_covariance = AHRS_StateVector::CovarianceMatrix::Zero();
    ahrs.process_noise_root_covariance.diagonal() <<
        1e-4 * UKF::Vector<3>::Ones(),
        1e-1 * UKF::Vector<3>::Ones();

    /* Initialise scale factor and bias errors. */
    ahrs_errors.state.set_field<AccelerometerBias>(UKF::Vector<3>(0, 0, 0));
    ahrs_errors.state.set_field<GyroscopeBias>(UKF::Vector<3>(0, 0, 0));

    /* Initialise scale factor and bias error covariance. */
    ahrs_errors.root_covariance = AHRS_SensorErrorVector::CovarianceMatrix::Zero();
    ahrs_errors.root_covariance.diagonal() <<
        0.8 * UKF::Vector<3>::Ones(),
        0.02 * UKF::Vector<3>::Ones();

    /* Set measurement noise covariance. */
    ahrs_errors.measurement_root_covariance << ahrs.measurement_root_covariance;

    /*
    Set bias error process noise – this is derived from bias instability.

    Bias instability is actually characterised as a 1/f flicker noise rather
    than the white noise (which is what we're specifying using the process
    noise covariance), so these are tuned by hand to values which allow the
    filter to track biases over time, but not change too quickly.
    */
    ahrs_errors.process_noise_root_covariance = AHRS_SensorErrorVector::CovarianceMatrix::Zero();
    ahrs_errors.process_noise_root_covariance.diagonal() <<
        1e-5f * UKF::Vector<3>::Ones(),
        1e-7f * UKF::Vector<3>::Ones();
}

void ukf_set_attitude(real_t w, real_t x, real_t y, real_t z) {
    ahrs.state.set_field<Attitude>(UKF::Quaternion(w, x, y, z));
}

void ukf_set_angular_velocity(real_t x, real_t y, real_t z) {
    ahrs.state.set_field<AngularVelocity>(UKF::Vector<3>(x, y, z));
}

void ukf_get_state(struct ukf_state_t *in) {
    in->attitude[0] = ahrs.state.get_field<Attitude>().x();
    in->attitude[1] = ahrs.state.get_field<Attitude>().y();
    in->attitude[2] = ahrs.state.get_field<Attitude>().z();
    in->attitude[3] = ahrs.state.get_field<Attitude>().w();
    in->angular_velocity[0] = ahrs.state.get_field<AngularVelocity>()[0];
    in->angular_velocity[1] = ahrs.state.get_field<AngularVelocity>()[1];
    in->angular_velocity[2] = ahrs.state.get_field<AngularVelocity>()[2];
    in->acceleration[0] = acceleration[0];
    in->acceleration[1] = acceleration[1];
    in->acceleration[2] = acceleration[2];
}

void ukf_set_state(struct ukf_state_t *in) {
    ahrs.state.set_field<Attitude>(
        UKF::Quaternion(in->attitude[3], in->attitude[0], in->attitude[1], in->attitude[2]));
    ahrs.state.set_field<AngularVelocity>(
        UKF::Vector<3>(in->angular_velocity[0], in->angular_velocity[1], in->angular_velocity[2]));
}

void ukf_get_state_covariance(
    real_t state_covariance[AHRS_StateVector::covariance_size()*AHRS_StateVector::covariance_size()]) {
    Eigen::Map<typename AHRS_StateVector::CovarianceMatrix> covariance_map(state_covariance);
    covariance_map = ahrs.root_covariance * ahrs.root_covariance.transpose();
}

void ukf_get_state_covariance_diagonal(
    real_t state_covariance_diagonal[AHRS_StateVector::covariance_size()]) {
    Eigen::Map<UKF::Vector<AHRS_StateVector::covariance_size()>> covariance_map(state_covariance_diagonal);
    covariance_map = (ahrs.root_covariance * ahrs.root_covariance.transpose()).diagonal();
}

void ukf_get_state_error(struct ukf_state_error_t *in) {
    AHRS_StateVector::StateVectorDelta state_error;
    state_error = (ahrs.root_covariance * ahrs.root_covariance.transpose()).cwiseAbs().rowwise().sum().cwiseSqrt();

    in->attitude[0] = state_error[0];
    in->attitude[1] = state_error[1];
    in->attitude[2] = state_error[2];
    in->angular_velocity[0] = state_error[3];
    in->angular_velocity[1] = state_error[4];
    in->angular_velocity[2] = state_error[5];
}

/*
This assumes accelerometer, gyroscope and magnetometer measurements are
present and in that order.
*/
void ukf_get_innovation(struct ukf_innovation_t *in) {
    in->accel[0] = ahrs.innovation[0];
    in->accel[1] = ahrs.innovation[1];
    in->accel[2] = ahrs.innovation[2];
    in->gyro[0] = ahrs.innovation[3];
    in->gyro[1] = ahrs.innovation[4];
    in->gyro[2] = ahrs.innovation[5];
}

void ukf_sensor_clear() {
    meas = AHRS_MeasurementVector();
}

void ukf_sensor_set_accelerometer(real_t x, real_t y, real_t z) {
    meas.set_field<Accelerometer>(UKF::Vector<3>(x, y, z));
}

void ukf_sensor_set_gyroscope(real_t x, real_t y, real_t z) {
    meas.set_field<Gyroscope>(UKF::Vector<3>(x, y, z));
}

void ukf_set_params(struct ukf_sensor_params_t *in) {
    ahrs.measurement_root_covariance <<
        std::sqrt(in->accel_covariance[0]), std::sqrt(in->accel_covariance[1]), std::sqrt(in->accel_covariance[2]),
        std::sqrt(in->gyro_covariance[0]), std::sqrt(in->gyro_covariance[1]), std::sqrt(in->gyro_covariance[2]);
    ahrs_errors.measurement_root_covariance << ahrs.measurement_root_covariance;
}

void ukf_iterate(float dt) {
    /*
    Split the parameter estimation filter into a priori and a posteriori
    steps, to reduce the CPU load each iteration.
    */
    static int step = 0;

    switch(step++) {
    case 0:
        /*
            The time delta is not used by the parameter estimation filter, so
            there's no need to adjust it.
            */
        ahrs_errors.a_priori_step();
        ahrs_errors.innovation_step(meas, ahrs.state);
        break;
    case 1:
        ahrs_errors.a_posteriori_step();

        UKF::Vector<3> temp;
        temp = ahrs_errors.state.get_field<AccelerometerBias>();
        temp[0] = std::max(real_t(-G_ACCEL/4.0), std::min(real_t(G_ACCEL/4.0), temp[0]));
        temp[1] = std::max(real_t(-G_ACCEL/4.0), std::min(real_t(G_ACCEL/4.0), temp[1]));
        temp[2] = std::max(real_t(-G_ACCEL/4.0), std::min(real_t(G_ACCEL/4.0), temp[2]));
        ahrs_errors.state.set_field<AccelerometerBias>(temp);

        step = 0;
        break;
    }

    /*
    Do a normal iteration for the AHRS filter, with the current state of
    the parameter estimation filter as the measurement input.
    */
    ahrs.a_priori_step(dt);
    ahrs.innovation_step(meas, ahrs_errors.state);
    ahrs.a_posteriori_step();

    acceleration << meas.get_field<Accelerometer>() - ahrs_errors.state.get_field<AccelerometerBias>() -
                        (ahrs.state.get_field<Attitude>() * UKF::Vector<3>(0, 0, -G_ACCEL));
}

void ukf_set_process_noise(real_t process_noise_covariance[AHRS_StateVector::covariance_size()]) {
    Eigen::Map<typename AHRS_StateVector::StateVectorDelta> covariance_map(process_noise_covariance);
    ahrs.process_noise_root_covariance = AHRS_StateVector::CovarianceMatrix::Zero();
    ahrs.process_noise_root_covariance.diagonal() << covariance_map;
    ahrs.process_noise_root_covariance = ahrs.process_noise_root_covariance.llt().matrixU();
}

void ukf_get_parameters(struct ukf_sensor_errors_t *in) {
    in->accel_bias[0] = ahrs_errors.state.get_field<AccelerometerBias>()[0];
    in->accel_bias[1] = ahrs_errors.state.get_field<AccelerometerBias>()[1];
    in->accel_bias[2] = ahrs_errors.state.get_field<AccelerometerBias>()[2];
    in->gyro_bias[0] = ahrs_errors.state.get_field<GyroscopeBias>()[0];
    in->gyro_bias[1] = ahrs_errors.state.get_field<GyroscopeBias>()[1];
    in->gyro_bias[2] = ahrs_errors.state.get_field<GyroscopeBias>()[2];
}

void ukf_get_parameters_error(struct ukf_sensor_errors_t *in) {
    AHRS_SensorErrorVector::StateVectorDelta parameters_error;
    parameters_error =
        (ahrs_errors.root_covariance * ahrs_errors.root_covariance.transpose()).cwiseAbs().rowwise().sum().cwiseSqrt();

    in->accel_bias[0] = parameters_error[0];
    in->accel_bias[1] = parameters_error[1];
    in->accel_bias[2] = parameters_error[2];
    in->gyro_bias[0] = parameters_error[3];
    in->gyro_bias[1] = parameters_error[4];
    in->gyro_bias[2] = parameters_error[5];
}

uint32_t ukf_config_get_state_dim() {
    return AHRS_StateVector::covariance_size();
}

uint32_t ukf_config_get_measurement_dim() {
    return AHRS_MeasurementVector::max_size();
}

enum ukf_precision_t ukf_config_get_precision() {
    if(sizeof(real_t) == 8) {
        return UKF_PRECISION_DOUBLE;
    } else {
        return UKF_PRECISION_FLOAT;
    }
}

MotionRunner::MotionRunner() :
    stop_signal(false),
    attitude(1, 0, 0, 0)
{
    matrix_imu_to_qt_axis_swap.fill(0.0);
    matrix_imu_to_qt_axis_swap(0, 1) = 1.0;
    matrix_imu_to_qt_axis_swap(1, 2) = 1.0;
    matrix_imu_to_qt_axis_swap(2, 0) = 1.0;
    matrix_imu_to_qt_axis_swap_tr.fill(0.0);
    matrix_imu_to_qt_axis_swap_tr(1, 0) = 1.0;
    matrix_imu_to_qt_axis_swap_tr(2, 1) = 1.0;
    matrix_imu_to_qt_axis_swap_tr(0, 2) = 1.0;
    //matrix_imu_to_qt_axis_mirror.fill(0.0);
    //matrix_imu_to_qt_axis_mirror(0, 0) = 1.0;
    //matrix_imu_to_qt_axis_mirror(1, 1) = 1.0;
    //matrix_imu_to_qt_axis_mirror(2, 2) = 1.0;
    //matrix_imu_to_qt_prerotate = QQuaternion::fromEulerAngles(180.0, 0.0, 0.0).toRotationMatrix();

    //matrix_qt_to_3d_axis_swap.fill(0.0);
    //matrix_qt_to_3d_axis_swap(0, 0) = 1.0;
    //matrix_qt_to_3d_axis_swap(1, 1) = 1.0;
    //matrix_qt_to_3d_axis_swap(2, 2) = 1.0;
    //matrix_qt_to_3d_axis_swap_tr.fill(0.0);
    //matrix_qt_to_3d_axis_swap_tr(0, 0) = 1.0;
    //matrix_qt_to_3d_axis_swap_tr(1, 1) = 1.0;
    //matrix_qt_to_3d_axis_swap_tr(2, 2) = 1.0;
    //matrix_qt_to_3d_axis_mirror.fill(0.0);
    //matrix_qt_to_3d_axis_mirror(0, 0) = 1.0;
    //matrix_qt_to_3d_axis_mirror(1, 1) = 1.0;
    //matrix_qt_to_3d_axis_mirror(2, 2) = 1.0;
    matrix_qt_to_3d_prerotate = QQuaternion::fromEulerAngles(180.0, 0.0, 0.0).toRotationMatrix();
}

MotionRunner::~MotionRunner(void)
{
    sendStopSignal();
    wait();
}

void MotionRunner::run(void)
{
    int32_t ret = 0;
    struct iio_context *ctx;
    struct iio_device *accl_dev, *gyro_dev;
    struct iio_channel *accl_x, *accl_y, *accl_z, *gyro_x, *gyro_y, *gyro_z;
    struct iio_buffer *accl_buffer, *gyro_buffer;
    bool accl_sampled = false, gyro_sampled = false;

    auto iio_ctx_cleaner = [this](struct iio_context ** sp) {
        iio_context_destroy(*sp);
    };
    auto iio_buffer_cleaner = [this](struct iio_buffer ** sp) {
        iio_buffer_destroy(*sp);
    };

    int epoll_fd;
    struct epoll_event ev, epoll_events[epoll_event_max];
    struct timespec epoll_timeout;

    ctx = iio_create_default_context();
    if (!ctx) {
        qWarning("iio context");
        return;
    }
    std::unique_ptr<struct iio_context *, decltype(iio_ctx_cleaner)>
        ctx_guard(&ctx, iio_ctx_cleaner);

    accl_dev = iio_context_find_device(ctx, accl_dev_name);
    gyro_dev = iio_context_find_device(ctx, gyro_dev_name);
    if (!accl_dev || !gyro_dev) {
        qWarning("iio device");
        return;
    }
    accl_x = iio_device_find_channel(accl_dev, accl_x_name, false);
    if (!accl_x) {
        qWarning("iio channel");
        return;
    }
    iio_channel_enable(accl_x);
    accl_y = iio_device_find_channel(accl_dev, accl_y_name, false);
    if (!accl_y) {
        qWarning("iio channel");
        return;
    }
    iio_channel_enable(accl_y);
    accl_z = iio_device_find_channel(accl_dev, accl_z_name, false);
    if (!accl_z) {
        qWarning("iio channel");
        return;
    }
    iio_channel_enable(accl_z);
    gyro_x = iio_device_find_channel(gyro_dev, gyro_x_name, false);
    if (!gyro_x) {
        qWarning("iio channel");
        return;
    }
    iio_channel_enable(gyro_x);
    gyro_y = iio_device_find_channel(gyro_dev, gyro_y_name, false);
    if (!gyro_y) {
        qWarning("iio channel");
        return;
    }
    iio_channel_enable(gyro_y);
    gyro_z = iio_device_find_channel(gyro_dev, gyro_z_name, false);
    if (!gyro_z) {
        qWarning("iio channel");
        return;
    }
    iio_channel_enable(gyro_z);

    accl_buffer = iio_device_create_buffer(accl_dev, 1, false);
    if (!accl_buffer) {
        qWarning("iio buffer");
        return;
    }
    std::unique_ptr<struct iio_buffer *, decltype(iio_buffer_cleaner)>
        accl_buffer_guard(&accl_buffer, iio_buffer_cleaner);

    gyro_buffer = iio_device_create_buffer(gyro_dev, 1, false);
    if (!gyro_buffer) {
        qWarning("iio buffer");
        return;
    }
    std::unique_ptr<struct iio_buffer *, decltype(iio_buffer_cleaner)>
        gyro_buffer_guard(&gyro_buffer, iio_buffer_cleaner);

    ret = iio_buffer_set_blocking_mode(accl_buffer, false);
    if (ret) {
        qWarning() << "accl_buffer non-blocking";
        return;
    }

    ret = iio_buffer_set_blocking_mode(gyro_buffer, false);
    if (ret) {
        qWarning() << "gyro_buffer non-blocking";
        return;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        qWarning() << "epoll_fd";
        return;
    }

    ev.events = EPOLLIN;
    ev.data.fd = iio_buffer_get_poll_fd(accl_buffer);
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);
    if (ret) {
        qWarning() << "epoll_ctl";
        return;
    }
    ev.events = EPOLLIN;
    ev.data.fd = iio_buffer_get_poll_fd(gyro_buffer);
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev);
    if (ret) {
        qWarning() << "epoll_ctl";
        return;
    }

    qCDebug(lc_motion) << "motion sensor initialized";

    ukf_init();

    struct ukf_state_t state;
    struct ukf_state_error_t state_e;
    struct ukf_innovation_t inno;
    struct ukf_sensor_errors_t sensor_p;
    struct ukf_sensor_errors_t sensor_e;
    ukf_get_state(&state);
    ukf_get_state_error(&state_e);
    //ukf_get_parameters(&sensor_p);
    //ukf_get_parameters_error(&sensor_e);
    QDateTime time_stamp = QDateTime::currentDateTime();

    qCDebug(lc_motion) << "motion runner ready";
    while (!getStopSignal()) {
        int nfds = epoll_wait(epoll_fd, epoll_events, epoll_event_max, 1000);
        if (nfds < 0) {
            qWarning() << "epoll wait timed out";
            break;
        }
        for (int j = 0; j < nfds; ++j) {
            if (epoll_events[j].data.fd == iio_buffer_get_poll_fd(accl_buffer) &&
                (epoll_events[j].events & EPOLLIN)) {
                int size = iio_buffer_refill(accl_buffer);
                if (size < 0) {
                    qWarning() << "iio refill" << size;
                } else {
                    int16_t ax, ay, az;
                    iio_channel_convert(accl_x, &ax, iio_buffer_first(accl_buffer, accl_x));
                    iio_channel_convert(accl_y, &ay, iio_buffer_first(accl_buffer, accl_y));
                    iio_channel_convert(accl_z, &az, iio_buffer_first(accl_buffer, accl_z));
                    az = - az;
                    //qCDebug(lc_motion) << "accl data: " << "ax"<< ax << "\tay"<< ay << "\taz"<< az;
                    ukf_sensor_set_accelerometer(
                        ax * 16.0 * G_ACCEL / 32768.0,
                        ay * 16.0 * G_ACCEL / 32768.0,
                        az * 16.0 * G_ACCEL / 32768.0
                        );
                    accl_sampled = true;
                }
            } else if (epoll_events[j].data.fd == iio_buffer_get_poll_fd(gyro_buffer) &&
                (epoll_events[j].events & EPOLLIN)) {
                int size = iio_buffer_refill(gyro_buffer);
                if (size < 0) {
                    qWarning() << "iio refill" << size;
                } else {
                    int16_t gx, gy, gz;
                    iio_channel_convert(gyro_x, &gx, iio_buffer_first(gyro_buffer, gyro_x));
                    iio_channel_convert(gyro_y, &gy, iio_buffer_first(gyro_buffer, gyro_y));
                    iio_channel_convert(gyro_z, &gz, iio_buffer_first(gyro_buffer, gyro_z));
                    //qCDebug(lc_motion) << "gyro data: " << "gx"<< gx << "\tgy"<< gy << "\tgz"<< gz;
                    gx = - gx;
                    ukf_sensor_set_gyroscope(
                        gx * 2000.0 * PI / 180.0 / 32768.0,
                        gy * 2000.0 * PI / 180.0 / 32768.0,
                        gz * 2000.0 * PI / 180.0 / 32768.0
                        );
                    gyro_sampled = true;
                }
            }

            if (accl_sampled && gyro_sampled) {
                accl_sampled = gyro_sampled = false;
                QDateTime now = QDateTime::currentDateTime();
                //qCDebug(lc_motion) << "motion ukf interval" << time_stamp.msecsTo(now) << "ms";
                time_stamp = now;
                ukf_set_state(&state);
                ukf_iterate(1.0 / 50.0);
                ukf_sensor_clear();
                ukf_get_state(&state);
                ukf_get_state_error(&state_e);
                //ukf_get_innovation(&inno);
                //ukf_get_parameters(&sensor_p);
                //ukf_get_parameters_error(&sensor_e);

                /**
                 * UKF result
                 */
                QMatrix3x3 rotation_matrix = QQuaternion(
                    state.attitude[0], state.attitude[1], state.attitude[2], state.attitude[3]
                ).toRotationMatrix();

                /**
                 * Adapt IMU axis to Qt
                 *
                 * (XZY) -> (ZXY)
                 *
                 * IMU axis definition:
                 *   X(IMU): Roll
                 *   Z(IMU): Pitch
                 *   Y(IMU): Yaw
                 *
                 * What we need:
                 *   Y(IMU): Roll
                 *   X(IMU): Pitch
                 *   Z(IMU): Yaw
                 *
                 * Qt axis definition:
                 *   Z(Qt): Roll
                 *   X(Qt): Pitch
                 *   Y(Qt): Yaw
                 *
                 * Qt treat EulerAngles in this order: Roll-Pitch-Yaw.
                 */
                rotation_matrix = matrix_imu_to_qt_axis_swap_tr * rotation_matrix * matrix_imu_to_qt_axis_swap;
                //{
                //    float roll, pitch, yaw;
                //    QQuaternion::fromRotationMatrix(rotation_matrix).getEulerAngles(&pitch, &yaw, &roll);
                //    qCDebug(lc_motion) << "Qt  (r:" << roll << "\tp:" << pitch << "  \ty:" << yaw << ")";
                //}

                if (TellTalesModel::instance()->settingsImuChaseView()) {
                    float roll, pitch, yaw;
                    QQuaternion::fromRotationMatrix(rotation_matrix).getEulerAngles(&pitch, &yaw, &roll);
                    rotation_matrix = QQuaternion::fromEulerAngles(pitch, 180.0f, roll).toRotationMatrix();
                }

                /**
                 * Adapt Qt axis to 3D
                 *
                 * (ZXY) -> (ZXY)
                 *
                 * Qt axis definition:
                 *   Z(Qt): Roll
                 *   X(Qt): Pitch
                 *   Y(Qt): Yaw
                 *
                 * 3D model axis definition:
                 *   Z(Qt): Roll
                 *   X(Qt): Pitch
                 *   Y(Qt): Yaw
                 *
                 * Qt treat EulerAngles in this order: Roll-Pitch-Yaw.
                 *
                 * To show 3D model in chase view, Get Euler Angles and set Yaw axis to constant.
                 */
                //rotation_matrix = matrix_qt_to_3d_axis_swap_tr * rotation_matrix * matrix_qt_to_3d_axis_swap;
                rotation_matrix = rotation_matrix * matrix_qt_to_3d_prerotate;

                /**
                 * Publish attitude result
                 */
                attitude = QQuaternion::fromRotationMatrix(rotation_matrix);
                emit attitudeChanged(attitude);

            }
        }
    }
}

bool MotionRunner::getStopSignal(void)
{
    stop_mutex.lock();
    bool b = stop_signal;
    stop_mutex.unlock();
    return b;
}

void MotionRunner::setState(bool b)
{
    if (b) {
        if (b != isRunning())
            start();
    } else {
        if (b == isRunning())
            sendStopSignal();
    }
}

void MotionRunner::sendStopSignal(void)
{
    stop_mutex.lock();
    stop_signal = true;
    stop_mutex.unlock();
}

QQuaternion MotionRunner::getAttitude(void)
{
    return attitude;
}

MotionModel::MotionModel(QObject* parent) :
    QObject(parent),
    runner()
{
    if (TellTalesModel::instance()->settingsImuEnable()) {
        QObject::connect(&runner, &MotionRunner::attitudeChanged, this, &MotionModel::attitudeChanged, Qt::QueuedConnection);
        runner.setState(true);
    }
}

MotionModel::~MotionModel(void)
{
}

MotionModel* MotionModel::instance()
{
    static std::unique_ptr<MotionModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new MotionModel);
    }
    return s_instance.get();
}

QQuaternion MotionModel::getAttitude(void)
{
    return runner.getAttitude();
}
