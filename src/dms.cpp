/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "dms.h"
#include <QCoreApplication>
#include <QByteArray>
#include <iostream>

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_dms, "model.dms")

DMS::DMS(QObject *parent) :
    QThread(parent),
    stop_signal(false),
    is_connected(false),
    rx_mq_des(-1),
    is_person_detected(false),
    dms_warning(false)
{
    timer.setSingleShot(true);
    timer.setInterval(5000);
    QObject::connect(&timer, &QTimer::timeout, this, &DMS::onTimerOverflow);
    QObject::connect(this, &DMS::retryRequest,this, &DMS::onRetryRequest);
}

DMS::~DMS() {
    sendStopSignal();
    wait();
}

bool DMS::isConnected(void)
{
    return is_connected;
}

void DMS::setIsConnected(bool b)
{
    if (is_connected == b){
        return;
    }
    is_connected = b;
    emit connectedChanged(is_connected);
}

void DMS::connectQueue(void)
{
    constexpr struct mq_attr attr = {
        .mq_flags = O_NONBLOCK,
        .mq_maxmsg = queue_size,
        .mq_msgsize = queue_msg_size,
        .mq_curmsgs = 0,
    };
    uint32_t ret = 0;

    setIsConnected(false);

    qCDebug(lc_dms) << "chat runner trying to connect to mqueue";

    rx_mq_des = mq_open(rx_queue_name, O_RDONLY, queue_permission, NULL);
    if (rx_mq_des == -1) {
        qCDebug(lc_dms) << "message queue" << rx_queue_name << "does not exist";
        return;
    }
    qCDebug(lc_dms) << "message queue" << rx_queue_name << "connected";

    setIsConnected(true);
}

void DMS::run()
{
    int32_t ret = 0;
    std::unique_ptr<char[]> buffer(new char[queue_msg_size]);
    struct timespec mq_timeout;

    auto state_cleaner = [this](DMS* sp) {
        if (isConnected()) {
            qCDebug(lc_dms) << "chat runner close mq on exit";
            mq_close(rx_mq_des);
            rx_mq_des = -1;
        }
        setIsConnected(false);
        setPersonDetected(false);
        setDmsWarning(false);
        if (!getStopSignal()) {
            emit retryRequest(); // start timer to trigger retry
        }
        stop_signal = false;
    };
    std::unique_ptr<DMS, decltype(state_cleaner)>
        state_guard(this, state_cleaner);

    connectQueue();
    if (!isConnected()) {
        return;
    }

    qCDebug(lc_dms) << "dms runner listening on queue" << rx_queue_name;
    while (!getStopSignal()) {
        clock_gettime(CLOCK_REALTIME, &mq_timeout);
        mq_timeout.tv_nsec += 10000;
        int ret = mq_timedreceive(rx_mq_des, buffer.get(), 1024, NULL, &mq_timeout);
        if (ret > 0 && ret < 1024) {
            buffer.get()[ret] = '\0';
            QByteArray message(buffer.get());
            //qCDebug(lc_dms) << "Message received: " << message;
            processMessage(message);
        } else if (ret != -1) { // timeout
            qCDebug(lc_dms) << "mq receive error return" << ret;
        }
    }
}

void DMS::setState(bool b)
{
    if (b) {
        if (b == isRunning() || b == timer.isActive())
            return;
        else
            start();
    } else {
        if (b == isRunning())
            sendStopSignal();
        else if (b == timer.isActive())
            timer.stop();
    }
}

bool DMS::getStopSignal() {
    stop_mutex.lock();
    bool b = stop_signal;
    stop_mutex.unlock();
    return b;
}

void DMS::sendStopSignal() {
    stop_mutex.lock();
    stop_signal = true;
    stop_mutex.unlock();
}

void DMS::onRetryRequest(void)
{
    timer.start();
}

void DMS::onTimerOverflow() {
    qCDebug(lc_dms) << "DMS queue timer timeout";
    start();
}

void DMS::processMessage(const QByteArray &message) {
    qCDebug(lc_dms) << "Received signal: " << message;
    if (message == "0") {
        setPersonDetected(true);
        setDmsWarning(false);
    } else if (message == "1") {
        setPersonDetected(true);
        setDmsWarning(true);
    } else if (message == "2") {
        setPersonDetected(false);
        setDmsWarning(false);
    } else {
        qCDebug(lc_dms) << "Invalid signal received: " << message;
    }
}

void DMS::setDmsWarning(bool b)
{
    if (dms_warning == b) {
        return;
    }
    dms_warning = b;
    emit dmsWarningChanged(dms_warning);
}

bool DMS::isDmsWarning(void)
{
    return dms_warning;
}

void DMS::setPersonDetected(bool b)
{
    if (is_person_detected == b) {
        return;
    }
    is_person_detected = b;
    emit personDetectedChanged(is_person_detected);
}

bool DMS::isPersonDetected(void)
{
    return is_person_detected;
}
