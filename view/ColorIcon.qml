/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Effects
import TellTalesModel 1.0

Item {
    id: root
    required property url source
    required property color color

    Loader {
        id: gpu
        anchors.fill:parent
        active: true
        asynchronous: true
        visible: status === Loader.Ready

        Binding {
            target: gpu.item
            property: "source"
            value: root.source
            when: gpu.status === Loader.Ready
        }
        Binding {
            target: gpu.item
            property: "color"
            value: root.color
            when: gpu.status === Loader.Ready
        }

        Component.onCompleted: {
            gpu.setSource(TellTalesModel.gpuPresent ? "ColorIcon_gpu.qml" : "ColorIcon_nogpu.qml", {
                "source": root.source,
                "color": root.color,
            });
        }
    }
}
