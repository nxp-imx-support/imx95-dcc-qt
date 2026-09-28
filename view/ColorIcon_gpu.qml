/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Effects

Item {
    id: root
    required property url source
    required property color color

    Image {
        id: image
        width: root.width
        height: root.height
        anchors.fill: parent
        smooth: true
        visible: false
        source: root.source
    }

    Rectangle {
        id: light
        anchors.fill: parent
        visible: false
        color: root.color
    }

    MultiEffect {
        anchors.fill: parent
        source: light
        maskEnabled: true
        maskSource: image
        maskInverted: false
        maskThresholdMin: 0.5
        maskThresholdMax: 1.0
        maskSpreadAtMin: 1.0
    }

    // Component.onCompleted: {
    //     console.log("ColorIcon_gpu loaded");
    // }
}
