/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DCC_APP_DccKeyDistanceModel_H
#define DCC_APP_DccKeyDistanceModel_H

#include <QObject>
#include <QQmlEngine>
#include "DccCanBusDelegation.h"

class DccKeyDistanceModel final: public QObject, public DccCanBusDelegation {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool state READ state NOTIFY stateChanged)
    Q_PROPERTY(float distance READ distance NOTIFY distanceChanged)
    Q_PROPERTY(float accuracy READ accuracy NOTIFY accuracyChanged)

public:
    explicit DccKeyDistanceModel(DccCanBusHandler &handler, QObject *parent = nullptr);
    ~DccKeyDistanceModel(void);

    int onCanBusFrameReceived(DccCanBusHandler::CanFrame& frame) override;

    bool state() const;
    float distance() const;
    float accuracy() const;

signals:
    void stateChanged(bool b);
    void distanceChanged(float v);
    void accuracyChanged(float v);

private:
    static constexpr uint32_t NXP_DCC_CAN_ID = 0x100;

    bool m_state;
    float m_distance;
    float m_accuracy;

    void setState(bool b);
    void setDistance(float v);
    void setAccuracy(float v);
};

#endif /* DCC_APP_DccKeyDistanceModel_H */
