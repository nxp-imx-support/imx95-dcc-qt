/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DMS_H
#define DMS_H

#include <QObject>
#include <QVariant>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include <iostream>
#include <memory>

#include <fcntl.h>
#include <mqueue.h>
#include <sys/epoll.h>
#include <unistd.h>

class DMS : public QThread {
    Q_OBJECT

public:
    DMS(QObject *parent = nullptr);
    ~DMS();

    void run(void) override;
    void setState(bool b);
    bool isConnected(void);
    bool isPersonDetected(void);
    bool isDmsWarning(void);

signals:
    void connectedChanged(bool isConnected);
    void retryRequest(void);
    void dmsWarningChanged(int dmsWarning);
    void personDetectedChanged(bool isConnected);

private:

    bool dms_warning;

    // base control
    QMutex stop_mutex;
    bool stop_signal;

    // msg queue
    static constexpr char const *rx_queue_name = "/dms_queue";
    static constexpr int queue_size = 10;
    static constexpr int queue_msg_size = 1024;
    static constexpr int queue_permission = 0644;
    int rx_mq_des;
    QTimer timer;
    bool is_connected;
    bool is_person_detected;
    void setIsConnected(bool b);
    void setPersonDetected(bool b);
    void connectQueue(void);
    void processMessage(const QByteArray &message);
    void setDmsWarning(bool b);

    bool getStopSignal(void);
    void sendStopSignal(void);

private Q_SLOTS:
    void onRetryRequest(void);
    void onTimerOverflow(void);
};

#endif // DMS_H
