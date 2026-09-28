/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "DccFindMyModel.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_findmy, "model.find_my")

DccFindMyModel::DccFindMyModel(DccCanBusHandler &handler, QObject *parent) :
    QObject(parent),
    DccCanBusDelegation(handler, NXP_DCC_CAN_ID),
    m_active(false) {
}

DccFindMyModel::~DccFindMyModel(void) {

}

bool DccFindMyModel::active() const {
    return m_active;
}

void DccFindMyModel::setActive(bool active) {
    if (m_active != active) {
        m_active = active;
        emit activeChanged();
    }
}

int DccFindMyModel::onCanBusFrameReceived(DccCanBusHandler::CanFrame& frame) {
    if(frame.can_id != NXP_DCC_CAN_ID) {
        qCWarning(lc_findmy) << "Wrong Find My CAN ID";
        return -1;
    }
    int status = static_cast<int>(frame.data[0]);
    qCInfo(lc_findmy) << "FindMy callback invoked, status: " << status;

    setActive(status == 1);
    return 0;
}
