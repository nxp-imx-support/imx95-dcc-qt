/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import Style 1.0
import TellTalesModel 1.0

Rectangle {
    id: bubble

    required property var locConnected
    required property var locDistance
    required property var locAccuracy

    property string headerText: "Key Detection"

    radius: 16

    anchors.fill: parent
    color: "black"
    opacity: 0.75
    border {
        color: Style.accentBlue
        width: 3
    }

    Text {
        id: headerStr
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 5
        text: bubble.headerText
        font.bold: true
        font.pointSize: 20
        color: Style.lightPeriwinkle
        elide: Text.ElideMiddle
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: distance
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        anchors.left: parent.left
        anchors.leftMargin: 80
        text: "distance"
        color: Style.grey
        font.pointSize: 14
        font.family: "Sarabun"
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: distanceValue
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        anchors.left: distance.right
        anchors.leftMargin: 4
        anchors.right: distanceUnit.left
        anchors.rightMargin: 4
        text: {
            if (locConnected === false || locAccuracy <= 0.80) {
                return " ...";
            } else {
                return locDistance.toFixed(1);
            }
        }
        color: Style.lightPeriwinkle
        font.pointSize: 14
        font.family: "Sarabun"
        horizontalAlignment: Text.AlignHCenter
    }

    Text {
        id: distanceUnit
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        anchors.right: parent.right
        anchors.rightMargin: 80
        text: "m"
        color: Style.grey
        font.pointSize: 14
        font.family: "Sarabun"
        horizontalAlignment: Text.AlignLeft
    }

}
