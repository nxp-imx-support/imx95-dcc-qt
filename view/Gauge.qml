/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick

Item {

    id: root
    property bool leftOrientation: false
    property real value
    property int minAngle: 0
    property int needleOffsetX: 0
    property int needleOffsetY: 0
    property int highlightOffsetX: 0
    property int highlightOffsetY: 0
    property int maxValue : 1
    property int maxAngle : 200
    property alias textLabel: label.text
    property alias valueText: textValue.text
    property int labelHorizontalCenterOffset: 0
    property int labelVerticalOffset: 0
    property int valueVerticalCenterOffset: -7
    property int valueHorizontalCenterOffset: 0

    width: 340
    height: 340

    property int transformOriginX: leftOrientation ? 60 : 280

    property real scale: 1
    property int interval: maxValue > 140 ? 20 : maxValue > 50 ? 10 : 1
    property real animatedValue: value
    Behavior on animatedValue { NumberAnimation {
        easing.type: Easing.OutQuad
        duration: 500
        }
    }

    Image {
        id: bg
        width: implicitWidth
        height: implicitHeight
        anchors.centerIn: parent
        source: "qrc:/images/gauges/gauge-frame.png"
        transform: [
            Scale {
                origin.x: bg.implicitWidth / 2
                xScale: leftOrientation ? 1 : -1
            }
        ]
    }

    Image {
        id: highlight
        x: 112 + highlightOffsetX
        y: -5 + highlightOffsetY
        width: implicitWidth
        height: implicitHeight
        source: "qrc:/images/gauges/highlight-normal.png"
        opacity: 0.75

        transform: [
            Rotation {
                origin.x: root.width / 2 - highlight.x
                origin.y: root.height / 2 - highlight.y - 0.5
                angle: minAngle/0.0174532925 + 180 + animatedValue * (leftOrientation ? 1 : -1) * (maxAngle - minAngle) / maxValue
            }
        ]
    }

    Image {
        id: needle
        x: 171 - needle.width/2 + needleOffsetX
        y: 16 + needleOffsetY
        width: implicitWidth
        height: implicitHeight
        source: "qrc:/images/gauges/needle-normal.png"

        transform: [
            Rotation {
                origin.x: root.width / 2 - needle.x
                origin.y: root.height / 2 - needle.y
                angle: minAngle/0.0174532925 + 180 + animatedValue * (leftOrientation ? 1 : -1) * (maxAngle - minAngle) / maxValue
            }
        ]
    }

    Repeater {
        model: 13;
        delegate: Text {
            id: entry
            visible: index <= Math.ceil(maxValue / interval)
            property real angle: minAngle + 2 * Math.PI * index * interval / maxValue * (maxAngle - minAngle) / 360
            x: (root.width - width) / 2 + (root.leftOrientation ? -1 : 1) * Math.sin(angle) * 135
            y: (root.height - height) / 2 + Math.cos(angle) * 135
            text: index * interval
            color: Style.lightPeriwinkle
            font.pixelSize: 16
            font.bold: false
            font.family: "Sarabun"
            opacity: {
                var distance = Math.abs(animatedValue/interval - index);
                var alphaResult = 1.5 - (distance / 1.25);
                return Math.min(Math.max(alphaResult, 0), 1);
            }
            Behavior on opacity { NumberAnimation { duration: 150; } }
        }
    }

    Text {
        id: textValue
        anchors.centerIn: parent
        anchors.verticalCenterOffset: valueVerticalCenterOffset
        anchors.horizontalCenterOffset: valueHorizontalCenterOffset
        horizontalAlignment: Text.AlignHCenter
        text: animatedValue.toFixed(0)
        color: Style.lightPeriwinkle
        font.pixelSize: 64
        font.bold: true
        font.family: "Sarabun"
    }

    Text {
        id: label
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: labelHorizontalCenterOffset
        anchors.topMargin: labelVerticalOffset
        anchors.top: textValue.bottom
        color: Style.lightPeriwinkle
        font.pixelSize: 20
        font.bold: false
        font.family: "Sarabun"
    }
}
