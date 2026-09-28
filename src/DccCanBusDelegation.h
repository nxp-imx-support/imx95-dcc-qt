/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DCC_APP_DCCCANBUSDELEGATION_H
#define DCC_APP_DCCCANBUSDELEGATION_H

#include <type_traits>
#include <functional>

#include "DccCanBusHandler.h"

class DccCanBusDelegation {
public:

    DccCanBusDelegation(DccCanBusHandler &handler, uint32_t frame_id) :
        handler(handler),
        frameId(frame_id) {

        // Register the delegation with the DccCanBusHandler
        handler.delegate(this);
    }
    DccCanBusDelegation(const DccCanBusDelegation&) = delete;

    virtual ~DccCanBusDelegation() {
        // Unregister from the handler on destruction
        handler.undelegate(this);
    }

    uint32_t FrameId(void) {
        return frameId;
    }

    virtual int onCanBusFrameReceived(DccCanBusHandler::CanFrame& frame) = 0;

private:
    DccCanBusHandler &handler;
    uint32_t frameId;
};

#endif /* DCC_APP_DCCCANBUSDELEGATION_H */
