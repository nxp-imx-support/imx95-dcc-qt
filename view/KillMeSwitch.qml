/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Shapes
import Style 1.0

Item {
    id: root

    property color activeColor: Style.highlighterRed
    property color inactiveColor: Style.darkBlue
    property color titleColor: Qt.darker(Style.lightPeriwinkle, 1.35)

    property bool triggered: false

    signal clicked()

    Button {
        id: killMeButton
        width: 100
        height: 100
        enabled: !root.triggered

        onPressed: {
            delayAnim.restart();
        }

        onReleased: {
            delayAnim.stop();
        }

        background: Rectangle {
            id: buttonBackground
            radius: width / 2
            color: "transparent"
            border {
                color: killMeButton.pressed ? Qt.darker(titleColor, 1.35): titleColor
                width: 5
            }

            ColorIcon {
                id: buttonIcon
                height: parent.height * 0.7
                width: parent.width * 0.7

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                source: "qrc:/images/media/skull-crossbones.svg"
                color: root.triggered ? root.activeColor : root.inactiveColor

            }

            Shape {
                smooth: true

                ShapePath {
                    fillColor: "transparent"
                    strokeColor: root.activeColor
                    strokeStyle: ShapePath.SolidLine
                    strokeWidth: 5
                    capStyle: ShapePath.FlatCap
                    PathAngleArc {
                        id: progressArc
                        centerX: killMeButton.width / 2
                        centerY: killMeButton.height / 2
                        radiusX: killMeButton.width / 2 - 2
                        radiusY: killMeButton.height / 2 - 2
                        startAngle: -90
                        sweepAngle: 0

                        NumberAnimation on sweepAngle {
                            id: delayAnim
                            from: 0
                            to: 360
                            duration: 2000
                            alwaysRunToEnd: false
                            running: false

                            onFinished: {
                                triggered = true;
                                root.clicked();
                            }
                            onStopped: {
                                killMeButton.enabled = false;
                                retractAnim.from = progressArc.sweepAngle;
                                retractAnim.to = 0
                                retractAnim.duration = 1000 * progressArc.sweepAngle / 360;
                                retractAnim.start();
                            }
                        }

                        NumberAnimation on sweepAngle {
                            id: retractAnim
                            alwaysRunToEnd: false
                            running: false
                            easing.type: Easing.OutBounce

                             onStopped: {
                                killMeButton.enabled = true;
                             }
                        }
                    }
                }
            }
        }
    }
}
