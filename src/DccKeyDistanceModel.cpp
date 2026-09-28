/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "DccKeyDistanceModel.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_key_dist, "model.key_dist")

DccKeyDistanceModel::DccKeyDistanceModel(DccCanBusHandler &handler, QObject *parent) :
    QObject(parent),
    DccCanBusDelegation(handler, NXP_DCC_CAN_ID),
    m_state(false),
    m_distance(0),
    m_accuracy(0) {
}

DccKeyDistanceModel::~DccKeyDistanceModel() {
}

bool DccKeyDistanceModel::state() const {
    return m_state;
}

float DccKeyDistanceModel::distance() const {
    return m_distance;
}

float DccKeyDistanceModel::accuracy() const {
    return m_accuracy;
}

void DccKeyDistanceModel::setState(bool state) {
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void DccKeyDistanceModel::setDistance(float distance) {
    if (m_distance != distance) {
        m_distance = distance;
        emit distanceChanged(m_distance);
    }
}

void DccKeyDistanceModel::setAccuracy(float accuracy) {
    if (m_accuracy != accuracy) {
        m_accuracy = accuracy;
        emit accuracyChanged(m_accuracy);
    }
}

int DccKeyDistanceModel::onCanBusFrameReceived(DccCanBusHandler::CanFrame& frame) {
    if(frame.can_id != NXP_DCC_CAN_ID) {
        qCWarning(lc_key_dist) << "Wrong Key Distance CAN ID" << frame.can_id;
        return -1;
    }
    // FIXME: make sure the bits are correct
    uint32_t accuracy = static_cast<int>(frame.data[0]);
    uint32_t distance = (static_cast<int>(frame.data[2]) << 8) | frame.data[3];

    qCDebug(lc_key_dist) << "KeyDistance callback invoked, distance: " << distance
                     << ", accuracy: " << accuracy;

    setDistance(distance / 100.0f);
    setAccuracy(accuracy / 100.0f);
    if (accuracy > 90) {
        setState(true);
    } else {
        setState(false);
    }
    return 0;
}
