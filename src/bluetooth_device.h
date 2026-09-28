/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BLUETOOTH_DEVICE_H
#define BLUETOOTH_DEVICE_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QReadWriteLock>
#include <QtConcurrent>
#include <QPromise>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QtQml/qqmlregistration.h>
#include <QDebug>
#include <iostream>
#include <memory>
#include <algorithm>

#include "bluetoothctl/bluetoothctl.h"

class BtInfoRxThread: public QThread
{
    Q_OBJECT

public:
    BtInfoRxThread(void);
    ~BtInfoRxThread(void);
    void run(void) override;

Q_SIGNALS:
    void ForwardCtlInfo(enum control_cmd idx, bool status);
    void ForwardDevInfo(enum control_cmd idx, dev_info dev);
    void ForwardMediaInfo(enum player_cmd idx, player_info media);

private:

};

class BluetoothDeviceInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString deviceName READ getName NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceAddress READ getAddress NOTIFY deviceChanged)
    Q_PROPERTY(bool connectionState READ getConnectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(bool activeState READ getActiveState NOTIFY activeStateChanged)
    Q_PROPERTY(bool pairState READ getPairState NOTIFY pairStateChanged)
    Q_PROPERTY(int deviceType READ getDeviceType NOTIFY deviceTypeChanged)

    QML_ANONYMOUS

public:
    BluetoothDeviceInfo() = default;
    BluetoothDeviceInfo(const QString &name, const QString &addr);

    bool operator == (const BluetoothDeviceInfo &b) const;
    bool operator > (const BluetoothDeviceInfo &b) const;
    bool operator < (const BluetoothDeviceInfo &b) const;

    QString getAddress() const;
    QString getName() const;
    bool getConnectionState() const;
    bool getActiveState() const;
    bool getPairState() const;
    int getDeviceType() const;

    void setDevice(const QString &name, const QString &addr);
    void setConnectionState(bool newState);
    void setActiveState(bool newState);
    void setPairState(bool newState);
    void setDeviceType(int newType);

    Q_INVOKABLE void connect(void);
    Q_INVOKABLE void disconnect(void);
    Q_INVOKABLE void unpair(void);

Q_SIGNALS:
    void deviceChanged();
    void connectRequest(BluetoothDeviceInfo *src, bool newState);
    void pairRequest(BluetoothDeviceInfo *src, bool newState);
    void connectionStateChanged(bool newState);
    void activeStateChanged(bool newState);
    void pairStateChanged(bool newState);
    void deviceTypeChanged(int newType);

private:
    /* forbid copy */
    BluetoothDeviceInfo(const BluetoothDeviceInfo &b) = delete;

    //QBluetoothDeviceInfo device;
    QString device_name, device_addr;
    bool connection_state;
    bool active_state;
    bool pair_state;
    int device_type;
};

class BluetoothDevice: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<BluetoothDeviceInfo *> devicesList READ getDevices NOTIFY devicesUpdated)
    Q_PROPERTY(bool guiState MEMBER gui_state NOTIFY guiStateChanged)
    Q_PROPERTY(bool scanState MEMBER is_scanning NOTIFY scanStateChanged)

    QML_ELEMENT
    QML_SINGLETON

public:
    ~BluetoothDevice(void);
    static BluetoothDevice* instance();
    bool getScanstate();
    void setScanstate(bool state);

    BluetoothDeviceInfo *getConnectedPhoneDevice(void);
    BluetoothDeviceInfo *getConnectedHelmetDevice0(void);
    BluetoothDeviceInfo *getConnectedHelmetDevice1(void);

    BluetoothCtl* controller;
    BtInfoRxThread BtInfoReceiver;

Q_INVOKABLE void scanDevices();

Q_SIGNALS:
    void devicesUpdated();
    void guiStateChanged(bool b);
    void scanStateChanged(bool b);
    void phoneDeviceConnectionChanged(bool b);
    void headsetDeviceConnectionChanged(bool b0, bool b1);
    void sendCmd();
    void bluetoothAgentReady(void);

public Q_SLOTS:
    void onGuiStateChanged(bool b);
    void onDeviceConnectRequest(BluetoothDeviceInfo *src, bool newState);
    void onDevicePairRequest(BluetoothDeviceInfo *src, bool newState);
    void onCtlInfoUpdate(enum control_cmd idx, bool status);
    void onDevInfoUpdate(enum control_cmd idx, dev_info dev);

    // debug slot
    void testSignal(bool b);

private:
    explicit BluetoothDevice(QObject* parent = nullptr);

    QList<BluetoothDeviceInfo *> getDevices();

    bool is_scanning;
    bool gui_state;
    bool filter_devices;   // filter unnamed bt devices

    QList<BluetoothDeviceInfo *> devices;
    //QReadWriteLock devices_rwlock;

    BluetoothDeviceInfo *phone_device, *helmet_device_0, *helmet_device_1;

    void updateDeviceInfoConnectionState(BluetoothDeviceInfo *deviceInfo, bool state);
    void updateDeviceInfoConnectionState(const QString& addr, bool state);
    void insertDeviceInfo(BluetoothDeviceInfo *deviceInfo);
    void reorderDeviceInfo(BluetoothDeviceInfo *deviceInfo);

    void addDeviceInfo(dev_info* bt_device);
    void removeDeviceInfo(const QString& addr);
    // void waitPlayerReady(const QString addr);

    QFuture<dev_info *> fu_scan;
    QFutureWatcher<dev_info *> fw_scan;

    QFuture<bool> fu_connect;
    QFutureWatcher<bool> fw_connect;
    BluetoothDeviceInfo* ctx_connect;
    QTimer timer_connect;

    QFuture<bool> fu_pair;
    QFutureWatcher<bool> fw_pair;

    QFuture<QString> fu_notify;
    QFutureWatcher<QString> fw_notify;

private Q_SLOTS:
    void scanStopped(void);
};

#endif // BLUETOOTH_DEVICE_H
