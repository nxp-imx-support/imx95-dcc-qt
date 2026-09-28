/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Shapes

Item {
    id: root

    required property real speed
    required property real battery

    width: 640
    height: 720

    Image {
        id: bg_img

        width: 640
        height: 720
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }

        source: "qrc:/images/ebike/bg1.png"
    }

    Image {
        id: gauge_bg

        width: 480
        height: 480

        x: 73
        y: 75

        source: "qrc:/images/ebike/gauge.png"
    }

    Image {
        id: gauge_needle

        width: 400
        height: 400

        x: 110
        y: 120

        source: "qrc:/images/ebike/ebike_gauge_needle.png"

        transform: [
            Rotation {
                origin.x: gauge_needle.width / 2
                origin.y: gauge_needle.height / 2
                angle: {
                    var angle_0 = 150;
                    var angle_range = 240;
                    var max_speed = 40;

                    if (root.speed >= max_speed) {
                        return angle_0 + angle_range;
                    } else {
                        return angle_0 + angle_range * root.speed / max_speed;
                    }
                }

                Behavior on angle { NumberAnimation {
                    easing.type: Easing.OutQuad
                    duration: 500
                    }
                }
            }
        ]
    }

    Image {
        id: battery_line

        width: 470
        height: 240

        x: 78
        y: 315

        source: "qrc:/images/ebike/side.png"
    }

    Text {
        id: battery_label

        width: 133
        height: 94

        x: 270
        y: 385

        text:  Math.floor(root.battery * 100).toString() + "%"
        color: "#0080ff"
        font.family: "montserratMedium"
        font.bold: true
        font.pointSize: 20
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Shape {
        id: battery_arc

        width: 400
        height: 400

        x: 112
        y: 120

        smooth: true

        ShapePath {
            fillColor: "transparent"
            strokeColor: "#0080ff"
            strokeStyle: ShapePath.SolidLine
            strokeWidth: 8
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: 200
                centerY: 200
                radiusX: 188
                radiusY: 188
                startAngle: 132
                sweepAngle: {
                    var angle_max = -84;
                    return angle_max * battery;
                }
            }
        }
    }

    Text {
        id: gauge_label_1

        width: 266
        height: 117

        x: 177
        y: 245

        text: Math.floor(root.speed).toString()
        color: "#ffffff"
        font.family: "montserratMedium"
        font.bold: true
        font.pointSize: 50
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: gauge_label_2

        width: 291
        height: 104

        x: 165
        y: 310

        text: "km/h"
        color: "#ffffff"
        font.family: "montserratMedium"
        font.bold: true
        font.pointSize: 20
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
