/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DCC_APP_DccFindMyModel_H
#define DCC_APP_DccFindMyModel_H

#include <QObject>
#include <QQmlEngine>
#include "DccCanBusDelegation.h"

class DccFindMyModel final : public QObject, public DccCanBusDelegation {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    explicit DccFindMyModel(DccCanBusHandler &handler, QObject *parent = nullptr);
    ~DccFindMyModel(void);

    int onCanBusFrameReceived(DccCanBusHandler::CanFrame& frame) override;

    bool active() const;

signals:
    void activeChanged();

private:
    static constexpr uint32_t NXP_DCC_CAN_ID = 0x110;

    bool m_active = false;

    void setActive(bool b);
};

#endif /* DCC_APP_DccFindMyModel_H */
