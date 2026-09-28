/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DCC_APP_DCCCANBUSHANDLER_H
#define DCC_APP_DCCCANBUSHANDLER_H

#include <map>
#include <QObject>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>

/** forward declaration */
class DccCanBusDelegation;

class DccCanBusHandler : public QObject {
    Q_OBJECT

public:
    struct CanFrame {
        static constexpr size_t max_data_size = 8;

        uint32_t can_id;
        uint8_t can_dlc;
        uint8_t data[max_data_size];

        static CanFrame fromQCanBusFrame(const QCanBusFrame &qframe) {
            CanFrame frame = {};
            frame.can_id = qframe.frameId();
            frame.can_dlc = std::min(static_cast<size_t>(qframe.payload().size()), max_data_size);
            for (int i = 0; i < qframe.payload().size() && i < max_data_size; ++i) {
                frame.data[i] = static_cast<uint8_t>(qframe.payload()[i]);
            }
            return frame;
        }
    };

    DccCanBusHandler(std::string can_if_name, QObject *parent = nullptr);
    ~DccCanBusHandler();

    /* Delegations */
    int delegate(DccCanBusDelegation *inst);
    void undelegate(DccCanBusDelegation *inst);

private slots:
    void onFramesReceived();
    void onErrorOccurred(QCanBusDevice::CanBusError error);

private:
    /* CAN interface */
    std::string if_name;
    QCanBusDevice *can_device;
    bool is_connected;

    int connectDevice();
    void setupFilters();

    /* Delegations */
    std::map<int, DccCanBusDelegation *> delegations;

    /* Frame processing */
    void processFrame(const QCanBusFrame &frame);
};

#endif /* DCC_APP_DCCCANBUSHANDLER_H */
