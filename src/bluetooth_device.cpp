/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "bluetooth_device.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_bt, "model.bluetooth")

bool unamed_filter = true;

BluetoothDeviceInfo::BluetoothDeviceInfo(const QString &name, const QString &addr)
{
    device_name = name;
    device_addr = addr;
    connection_state = false;
    active_state = false;
    pair_state = false;
    device_type = -1;
}

bool BluetoothDeviceInfo::operator == (const BluetoothDeviceInfo &b) const
{
    return device_addr == b.getAddress();
}

bool BluetoothDeviceInfo::operator > (const BluetoothDeviceInfo &b) const
{
    if (connection_state != b.connection_state) {
        return connection_state;
    } else if (pair_state != b.pair_state) {
        return pair_state;
    } else {
        return (device_addr > b.device_addr);
    }
}

bool BluetoothDeviceInfo::operator < (const BluetoothDeviceInfo &b) const
{
    if (connection_state != b.connection_state) {
        return b.connection_state;
    } else if (pair_state != b.pair_state) {
        return b.pair_state;
    } else {
        return (device_addr < b.device_addr);
    }
}

QString BluetoothDeviceInfo::getAddress() const
{
    return device_addr;
}

QString BluetoothDeviceInfo::getName() const
{
    return device_name;
}

bool BluetoothDeviceInfo::getConnectionState() const
{
    return connection_state;
}

bool BluetoothDeviceInfo::getActiveState() const
{
    return active_state;
}

bool BluetoothDeviceInfo::getPairState() const
{
    return pair_state;
}

int BluetoothDeviceInfo::getDeviceType() const
{
    return device_type;
}

void BluetoothDeviceInfo::setDevice(const QString &name, const QString &addr)
{
    device_name = name;
    device_addr = addr;
    emit deviceChanged();
}

void BluetoothDeviceInfo::setConnectionState(bool newState)
{
    if (connection_state == newState) {
        return;
    }
    connection_state = newState;
    if (connection_state) {
        qCDebug(lc_bt) << "device" << device_name << "connected";
    } else {
        qCDebug(lc_bt) << "device" << device_name << "disconnected";
    }
    emit connectionStateChanged(connection_state);
}

void BluetoothDeviceInfo::setActiveState(bool newState)
{
    if (active_state == newState) {
        return;
    }
    active_state = newState;
    if (active_state) {
        qCDebug(lc_bt) << "device" << device_name << "active";
    } else {
        qCDebug(lc_bt) << "device" << device_name << "inavtive";
    }
    emit activeStateChanged(connection_state);
}

void BluetoothDeviceInfo::setPairState(bool newState)
{
    if (pair_state == newState) {
        return;
    }
    pair_state = newState;
    if (pair_state) {
        qCDebug(lc_bt) << "device" << device_name << "paired";
    } else {
        qCDebug(lc_bt) << "device" << device_name << "unpaired";
    }
    emit pairStateChanged(pair_state);
}

void BluetoothDeviceInfo::setDeviceType(int newType)
{
    if (device_type == newType) {
        return;
    }
    device_type = newType;
    qCDebug(lc_bt) << "device" << device_name << "type" << device_type;
    emit deviceTypeChanged(device_type);
}

void BluetoothDeviceInfo::connect(void)
{
    if (active_state || connection_state)
        return;
    emit connectRequest(this, true);
}

void BluetoothDeviceInfo::disconnect(void)
{
    if (!connection_state)
        return;
    emit connectRequest(this, false);
}
void BluetoothDeviceInfo::unpair(void)
{
    if(!pair_state)
        return;
    emit pairRequest(this, false);
}


/******************************************************************
 * BluetoothDevice
 * ****************************************************************/

BluetoothDevice::BluetoothDevice(QObject* parent) : QObject(parent)
{
    gui_state = false;
    is_scanning = false;
    filter_devices = DISABLE_BLE;

    phone_device = helmet_device_0 = helmet_device_1 = nullptr;

    BtInfoReceiver.start();

    QObject::connect(this, &BluetoothDevice::guiStateChanged, this, &BluetoothDevice::testSignal);
    QObject::connect(this, &BluetoothDevice::guiStateChanged, this, &BluetoothDevice::onGuiStateChanged);

    connect(&BtInfoReceiver, &BtInfoRxThread::ForwardCtlInfo, this, &BluetoothDevice::onCtlInfoUpdate, Qt::QueuedConnection);
    connect(&BtInfoReceiver, &BtInfoRxThread::ForwardDevInfo, this, &BluetoothDevice::onDevInfoUpdate, Qt::QueuedConnection);
    connect(&fw_scan, &QFutureWatcher<dev_info *>::finished, this, &BluetoothDevice::scanStopped);


    controller = new BluetoothCtl();
    controller->run();

    timer_connect.setSingleShot(true);
    timer_connect.setInterval(10000);
    connect(&timer_connect, &QTimer::timeout, [=](void) {
        if (ctx_connect) {
            ctx_connect->setActiveState(false);
            ctx_connect = nullptr;
            qCDebug(lc_bt) << "connect timeout, clear ctx_connect";
        }
    });
    connect(&fw_connect, &QFutureWatcher<bool>::finished, [=](void) {
        //qCDebug(lc_bt) << "connect/disconnect task finished";
    });
    connect(&fw_connect, &QFutureWatcher<bool>::resultReadyAt, [=](int index) {
        bool result = fw_connect.resultAt(index);
        //qCDebug(lc_bt) << "fw_connect task return " << result;
        if (!result && ctx_connect) {
            ctx_connect->setActiveState(false);
            ctx_connect = nullptr;
            qCDebug(lc_bt) << "fw_connect task failed, clear ctx_connect.";
        }
    });

    ctx_connect = nullptr;
}

BluetoothDevice* BluetoothDevice::instance()
{
    static std::unique_ptr<BluetoothDevice> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new BluetoothDevice);
    }
    return s_instance.get();
}

BluetoothDevice::~BluetoothDevice()
{
    delete controller;
    for (auto&& dev : devices) {
        delete dev;
    }
}

QList<BluetoothDeviceInfo *> BluetoothDevice::getDevices()
{
    auto ret = devices;
    return ret;
}

void BluetoothDevice::onGuiStateChanged(bool b)
{
    if (b) {
        qCDebug(lc_bt) << "bt scan start";
        scanDevices();
    }
    else {
        qCDebug(lc_bt) << "bt scan stop";
        scanStopped();
        return;
    }
}

void BluetoothDevice::onDeviceConnectRequest(BluetoothDeviceInfo *src, bool newState)
{
    std::string mac;

    if (fw_connect.isRunning() || ctx_connect) {
        qCDebug(lc_bt) << "cannot connect, another job is running";
        return;
    }
    if ((src->getDeviceType() != PHONE) && (src->getDeviceType() != HEADSET)) {
        qCDebug(lc_bt) << "cannot connect to unknown device";
        return;
    }
    if (newState) {
        if ((src->getDeviceType() == PHONE) && (phone_device) ) {
            qCDebug(lc_bt) << "cannot connect to another phone device";
            return;
        }
        if ((src->getDeviceType() == HEADSET) && (helmet_device_0) && (helmet_device_1) ) {
            qCDebug(lc_bt) << "cannot connect to another helmet device";
            return;
        }
    }

    /** store device ptr here for future use */
    ctx_connect = src;
    ctx_connect->setActiveState(true);
    mac = (ctx_connect->getAddress()).toStdString();

    if (newState) {
        /** do connect */
        fu_connect = QtConcurrent::task([=] (QPromise<bool> &promise) {
            // BluetoothDevice* bt = BluetoothDevice::instance();
            qCDebug(lc_bt) << "do connect task";
            controller->connect(mac.c_str());
            promise.addResult(true);
        }).spawn();
        fw_connect.setFuture(fu_connect);
    } else {
        /** do disconnect */
        fu_connect = QtConcurrent::task([=] (QPromise<bool> &promise) {
            // BluetoothDevice* bt = BluetoothDevice::instance();
            qCDebug(lc_bt) << "do disconnect task";
            controller->disconnect(mac.c_str());
            promise.addResult(true);
        }).spawn();
        fw_connect.setFuture(fu_connect);
    }

    timer_connect.start();
}

void BluetoothDevice::onDevicePairRequest(BluetoothDeviceInfo *src, bool newState)
{
    std::string mac;

    if (src->getActiveState()) {
        qCDebug(lc_bt) << "cannot (un)pair, device is active";
        return;
    }

    if (fw_pair.isRunning()) {
        qCDebug(lc_bt) << "cannot (un)pair, another job is running";
        return;
    }

    mac = (src->getAddress()).toStdString();

    if (newState) {
        /** do pair */
        qCDebug(lc_bt) << "paring is not supported. use connect instead";
    } else {
        /** do unpair */
        fu_pair = QtConcurrent::task([=] (QPromise<bool> &promise) {
            BluetoothDevice* bt = BluetoothDevice::instance();
            qCDebug(lc_bt) << "do unpair task";
            bt->controller->remove(mac.c_str());
            promise.addResult(true);
        }).spawn();
        fw_pair.setFuture(fu_pair);
    }
}

void BluetoothDevice::onCtlInfoUpdate(enum control_cmd type, bool status)
{
    switch (type) {
        case AGENT:
            controller->agent_registered = true;
            emit bluetoothAgentReady();
            // emit devicesUpdated();
            break;
        case DEFAULT_PLAYER:
            controller->player_registered = true;
            break;
        default:
            qCDebug(lc_bt) << "BluetoothDevice->onCtlInfoUpdate: unsupport CONTROL cmd " << type;
            break;
    }
}

void BluetoothDevice::onDevInfoUpdate(enum control_cmd type, dev_info dev)
{
    switch (type) {
        case ADD: {
            addDeviceInfo(&dev);
            break;
        }
        case CONNECT: {
            QString device_addr = QString(dev.address);
            updateDeviceInfoConnectionState(device_addr, dev.connected);
            break;
        }
        case REMOVE: {
            QString device_addr = QString(dev.address);
            removeDeviceInfo(device_addr);
            break;
        }

        default:
            qCDebug(lc_bt) << "BluetoothDevice->onDevInfoUpdate: unsupport CONTROL cmd " << type;
            break;
    }
}

bool BluetoothDevice::getScanstate()
{
    return is_scanning;
}

void BluetoothDevice::setScanstate(bool state)
{
    is_scanning = state;
}

BluetoothDeviceInfo *BluetoothDevice::getConnectedPhoneDevice(void)
{
    return phone_device;
}
BluetoothDeviceInfo *BluetoothDevice::getConnectedHelmetDevice0(void)
{
    return helmet_device_0;
}
BluetoothDeviceInfo *BluetoothDevice::getConnectedHelmetDevice1(void)
{
    return helmet_device_1;
}

void BluetoothDevice::scanDevices()
{
    if (is_scanning)
        return;

    is_scanning = true;

    controller->scan_on();

    emit scanStateChanged(is_scanning);
}

void BluetoothDevice::scanStopped(void)
{
    is_scanning = false;
    controller->scan_off();

    emit scanStateChanged(is_scanning);
}

void BluetoothDevice::testSignal(bool b)
{
    qCDebug(lc_bt) << "gui changed signal emit:" << b;
}

void BluetoothDevice::updateDeviceInfoConnectionState(BluetoothDeviceInfo *deviceInfo, bool state)
{
    assert (deviceInfo);

    bool state_changed = (deviceInfo->getConnectionState() != state);
    if (!state_changed) {
        goto ui_sync;
    }

    deviceInfo->setActiveState(true);

    if (state) {
        // TODO: add pair state notification
        // we do not have such notification, so set pair to ture if connected.
        deviceInfo->setPairState(true);
    }

    deviceInfo->setConnectionState(state);
    deviceInfo->setActiveState(false);
    reorderDeviceInfo(deviceInfo);
    //qCDebug(lc_bt) << "device state changed accidentally";

    if (state) {
        if (deviceInfo->getDeviceType() == PHONE) {
            if (!phone_device) {
                phone_device = deviceInfo;
                emit phoneDeviceConnectionChanged(phone_device);
            } else {
                qCDebug(lc_bt) << "err: phone device overflow";
            }
        } else if (deviceInfo->getDeviceType() == HEADSET) {
            if (!helmet_device_0) {
                helmet_device_0 = deviceInfo;
                emit headsetDeviceConnectionChanged(bool(helmet_device_0), bool(helmet_device_1));
            } else if (!helmet_device_1) {
                helmet_device_1 = deviceInfo;
                emit headsetDeviceConnectionChanged(bool(helmet_device_0), bool(helmet_device_1));
            } else {
                qCDebug(lc_bt) << "err: headset device overflow";
            }
        }
    } else {
        if (deviceInfo->getDeviceType() == PHONE) {
            if (phone_device == deviceInfo) {
                phone_device = nullptr;
                emit phoneDeviceConnectionChanged(bool(phone_device));
            } else {
                qCDebug(lc_bt) << "wrn: unknown phone disconnected";
            }
        } else if (deviceInfo->getDeviceType() == HEADSET) {
            if (helmet_device_0 == deviceInfo) {
                helmet_device_0 = helmet_device_1;
                helmet_device_1 = nullptr;
                emit headsetDeviceConnectionChanged(bool(helmet_device_0), bool(helmet_device_1));
            } else if (helmet_device_1 == deviceInfo) {
                helmet_device_1 = nullptr;
                emit headsetDeviceConnectionChanged(bool(helmet_device_0), bool(helmet_device_1));
            } else {
                qCDebug(lc_bt) << "wrn: unknown headset disconnected";
            }
        }
    }

ui_sync:
    // qCDebug(lc_bt) << "--- 3:  " << state_changed << "  " << controller->agent_registered;
    // if(controller->agent_registered == true)
    emit devicesUpdated();
}

void BluetoothDevice::updateDeviceInfoConnectionState(const QString& addr, bool state)
{
    BluetoothDeviceInfo* deviceInfo = nullptr;

    if (ctx_connect && ctx_connect->getAddress() == addr) {
        deviceInfo = ctx_connect;
        ctx_connect = nullptr;
        timer_connect.stop();
    } else {
        for (auto&& deviceIter : devices) {
            if (deviceIter->getAddress() == addr) {
                deviceInfo = deviceIter;
                break;
            }
        }
    }
    if (!deviceInfo) {
        qCDebug(lc_bt) << "notified device is not included in device list";
        return;
    }
    updateDeviceInfoConnectionState(deviceInfo, state);
}

void BluetoothDevice::insertDeviceInfo(BluetoothDeviceInfo *deviceInfo)
{
    for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
        if ((*deviceInfo) > (**iter)) {
            iter = devices.insert(iter, deviceInfo);
            return;
        }
    }

    devices.push_back(deviceInfo);

}

void BluetoothDevice::reorderDeviceInfo(BluetoothDeviceInfo *deviceInfo)
{
    if (devices.indexOf(deviceInfo) == -1) {
        qCDebug(lc_bt) << "abort reorder: not in devices list";
        return;
    }

    devices.removeAll(deviceInfo);
    insertDeviceInfo(deviceInfo);
}

void BluetoothDevice::addDeviceInfo(dev_info* bt_device)
{
    QString dev_name = QString(bt_device->name);
    QString dev_addr = QString(bt_device->address);

    qCDebug(lc_bt) << "new bt device name: " << dev_name << "adress: "<< dev_addr << " device type:" << bt_device->type;

    // the device removed
    if (dev_name == "" || dev_name == "unknown device 0")
        return;

    // filter added device
    for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
        if ((*iter)->getAddress() == dev_addr) {
            return;
        }
    }

    BluetoothDeviceInfo* deviceInfoPtr = new BluetoothDeviceInfo(std::move(dev_name), std::move(dev_addr));
    deviceInfoPtr->setDeviceType(bt_device->type);
    deviceInfoPtr->setPairState(bt_device->paired);
    insertDeviceInfo(deviceInfoPtr);

    qCDebug(lc_bt) << "qt debug: bt device nums: " << devices.size();
    updateDeviceInfoConnectionState(deviceInfoPtr, bt_device->connected);
    connect(deviceInfoPtr, &BluetoothDeviceInfo::pairRequest, this, &BluetoothDevice::onDevicePairRequest);
    connect(deviceInfoPtr, &BluetoothDeviceInfo::connectRequest, this, &BluetoothDevice::onDeviceConnectRequest);
}

void BluetoothDevice::removeDeviceInfo(const QString& addr)
{
    for (auto iter = devices.begin(); iter != devices.end(); ++iter) {
        if ((*iter)->getAddress() == addr) {
            if ((*iter)->getActiveState()) {
                /** do NOT remove active devices */
                qCDebug(lc_bt) << "cannot remove active device";
                return;
            }
            if ((*iter)->getConnectionState()) {
                updateDeviceInfoConnectionState((*iter), false);
            }
            delete (*iter);
            iter = devices.erase(iter);
            emit devicesUpdated();
            return;
        }
    }
}

/******************************************************************
 * BtInfoRxThread
 * ****************************************************************/

BtInfoRxThread::BtInfoRxThread(void)
{
    qCDebug(lc_bt) << "start recevice bt info";
}

BtInfoRxThread::~BtInfoRxThread(void)
{
    qCDebug(lc_bt) << "end recevice bt info";
}

void BtInfoRxThread::run(void)
{
    int msqid_rx;
    key_t key_rx;
    message_b2a msg;
    enum cmd_type type;
    enum control_cmd ctl_idx;
    enum player_cmd  ply_idx;

    dev_info dev = {
        .address = "",
        .name = "",
        .paired = false,
        .connected = false,
        .type = UNKNOWN,
    };

    player_info media = {
        .player_status = false,
        .title = "",
        .artist = "",
        .duration = 0,
        .position = 0,
    };


    // Generate info RX queue
    if ((key_rx = ftok(SIG_PATH, SIG_B2A)) == -1) {
        perror("BtInfoRxThread rx ftok");
    }

    if ((msqid_rx = msgget(key_rx, 0666 | IPC_CREAT)) == -1) {
        perror("BtInfoRxThread rx msgget");
    }

        printf("!!!! 2 key_rx = %d, msqid_rx = %d! \n", key_rx, msqid_rx);

    while (1) {

        if (msgrcv(msqid_rx, &msg, sizeof(msg.message), 0, 0) == -1) {
            perror("BtInfoRxThread msgrcv");
            return;
        }

        qCDebug(lc_bt) << "BtInfoRxThread: revice message type " << msg.mtype << "idx " << msg.message.idx;
        type = (enum cmd_type) msg.mtype;


        if( type == CONTROL) {
            ctl_idx = (enum control_cmd)msg.message.idx;
            switch (ctl_idx) {
                case AGENT:
                    qCDebug(lc_bt) << "BtInfoRxThread: agent on";
                    emit ForwardCtlInfo(ctl_idx, msg.message.agent);
                    break;
                case DEFAULT_PLAYER:
                    emit ForwardCtlInfo(ctl_idx, msg.message.default_player);
                    break;
                case ADD:
                case REMOVE:
                case CONNECT:
                    memcpy(&dev, &msg.message.dev,sizeof(dev_info));
                    emit ForwardDevInfo(ctl_idx, dev);
                    break;
            default:
                qCDebug(lc_bt) << "BtInfoRxThread: unsupport CONTROL info " << ctl_idx;
                break;
            }
        }

        if( type == PLAYER) {
            ply_idx = (enum player_cmd)msg.message.idx;
            switch (ply_idx) {
                case PLAYER_STATUS:
                case PLAYER_PROPERTY:
                    memcpy(&media, &msg.message.player,sizeof(player_info));
                    ForwardMediaInfo(ply_idx, media);
                    break;
            default:
                qCDebug(lc_bt) << "BtInfoRxThread: unsupport PLAYER info " << ctl_idx;
                break;
            }
        }

    }
}