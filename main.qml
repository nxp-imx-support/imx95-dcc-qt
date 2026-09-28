/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtMultimedia

import "view" as View

import DriveSimulation 1.0
import TellTalesModel 1.0

ApplicationWindow {
    id: window

    property int camWindowMargin: 90

    width: 1920
    height: 720
    visible: true
    title: "digital_cluster"
    color: "transparent"

    // Uncomment this line on board
    visibility: Window.FullScreen
    flags: Qt.FramelessWindowHint | Qt.Window

    View.EbikeUI {
        id: guage
        width: 640
        height: 720
        anchors.left: parent.left
        visible: TellTalesModel.showGauge

        speed: DriveSimulation.bikeSpeed / 3.0
        battery: DriveSimulation.fuelLevel / DriveSimulation.fuelCapacity
    }

    Rectangle {
        id: cluster
        width: 1280
        height: 720
        anchors.right: parent.right
        color: "transparent"

        View.TellTales {
            anchors.horizontalCenter: cluster.horizontalCenter
            y:16
        }

        Image {
            id: topLine
            source: "qrc:/images/top-line.png"
            anchors.horizontalCenter: parent.horizontalCenter
            y: 80
        }

        View.NormalMode {
            id: normalMode
            anchors.fill: cluster
        }
    }
}
