/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <QList>
#include <QVariant>
#include "DccCanBusHandler.h"
#include "DccCanBusDelegation.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_canbus, "model.canbus")

DccCanBusHandler::DccCanBusHandler(std::string can_if_name, QObject *parent) :
    QObject(parent),
    if_name(can_if_name),
    can_device(nullptr),
    is_connected(false) {

    QString errorString;

    // Create CAN bus device using SocketCAN plugin
    can_device = QCanBus::instance()->createDevice(
        QStringLiteral("socketcan"),
        QString::fromStdString(if_name),
        &errorString
    );

    if (!can_device) {
        qCCritical(lc_canbus) << "Failed to create CAN device: " << errorString;
        return;
    }

    // Connect signals
    connect(can_device, &QCanBusDevice::framesReceived,
            this, &DccCanBusHandler::onFramesReceived);
    connect(can_device, &QCanBusDevice::errorOccurred,
            this, &DccCanBusHandler::onErrorOccurred);

    // Connect to the CAN bus
    if (!can_device->connectDevice()) {
        qCCritical(lc_canbus) << "Failed to connect to CAN bus: "
                   << can_device->errorString();
        delete can_device;
        can_device = nullptr;
        return;
    }

    is_connected = true;
    qCInfo(lc_canbus) << "Successfully connected to CAN interface: " << QString::fromStdString(if_name);
}

DccCanBusHandler::~DccCanBusHandler() {
    if (can_device) {
        can_device->disconnectDevice();
        delete can_device;
        can_device = nullptr;
    }
}

void DccCanBusHandler::setupFilters() {
    if (!can_device || delegations.empty()) {
        return;
    }

    qCInfo(lc_canbus) << "Setting up CAN filters for " << delegations.size() << " delegations";

    QList<QCanBusDevice::Filter> filters;

    for (auto&& inst : delegations) {
        QCanBusDevice::Filter filter;
        filter.frameId = inst.second->FrameId();
        filter.frameIdMask = 0x7FF; // Standard 11-bit CAN ID mask
        filter.format = QCanBusDevice::Filter::MatchBaseFormat;
        filter.type = QCanBusFrame::DataFrame;

        filters.append(filter);

        qCInfo(lc_canbus) << "Added filter for CAN ID: " << inst.second->FrameId()
                  << ", Delegation: " << inst.second;
    }

    can_device->setConfigurationParameter(
        QCanBusDevice::RawFilterKey,
        QVariant::fromValue(filters)
    );
}

int DccCanBusHandler::delegate(DccCanBusDelegation *inst) {
    auto frame_id = inst->FrameId();

    auto result = delegations.try_emplace(frame_id, inst);

    if (result.second) {
        qCInfo(lc_canbus) << "Delegation " << inst << " registered for can_id: " << frame_id;
        setupFilters();
        return 0;
    } else {
        qCCritical(lc_canbus) << "Delegation " << inst << " registration failed for can_id: " << frame_id;
        return -1;
    }
}

void DccCanBusHandler::undelegate(DccCanBusDelegation *inst) {
    auto frame_id = inst->FrameId();

    auto it = delegations.find(frame_id);
    if (it != delegations.end() && it->second == inst) {
        delegations.erase(it);
        qCInfo(lc_canbus) << "Delegation " << inst << " unregistered for can_id: " << frame_id;
        setupFilters();
    } else {
        qCWarning(lc_canbus) << "Attempted to unregister unknown delegation " << inst
                     << " for can_id: " << frame_id;
    }
}

void DccCanBusHandler::onFramesReceived() {
    if (!can_device) {
        return;
    }

    while (can_device->framesAvailable()) {
        const QCanBusFrame frame = can_device->readFrame();

        if (!frame.isValid()) {
            qCWarning(lc_canbus) << "Received invalid CAN frame";
            continue;
        }

        processFrame(frame);
    }
}

void DccCanBusHandler::processFrame(const QCanBusFrame &frame) {
    quint32 frameId = frame.frameId();

    qCInfo(lc_canbus) << "CAN frame received: " << frameId
              << " DLC: " << frame.payload().size();

    if (delegations.find(frameId) != delegations.end()) {
        auto delegation = delegations.at(frameId);
        if (delegation == nullptr) {
            qCWarning(lc_canbus) << "CAN ID matched delegation with nullptr";
            return;
        }

        qCInfo(lc_canbus) << "CAN ID matched delegation " << delegation;

        DccCanBusHandler::CanFrame canFrame = CanFrame::fromQCanBusFrame(frame);
        int result = delegation->onCanBusFrameReceived(canFrame);
        if (result) {
            qCWarning(lc_canbus) << "Handler returned non-zero value: " << result;
        }
    } else {
        qCWarning(lc_canbus) << "Received unknown CAN ID: " << frameId;
    }
}

void DccCanBusHandler::onErrorOccurred(QCanBusDevice::CanBusError error) {
    if (!can_device) {
        return;
    }

    qCCritical(lc_canbus) << "CAN bus error occurred: "
               << can_device->errorString()
               << " (Error code: " << static_cast<int>(error) << ")";
}
