/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import Style 1.0

Item {
    id: root

    required property bool androidState
    required property bool commandState

    property color activeColor: Style.highlighterGreen
    property color inactiveColor: Style.darkBlue
    property color titleColor: Qt.darker(Style.lightPeriwinkle, 1.35)

    signal androidClicked()

    Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }

        Button {
            id: androidButton
            width: 100
            height: 100

            onClicked: root.androidClicked()

            background: Rectangle {
                id: buttonBackground
                radius: width / 2
                color: "transparent"
                border {
                    color: androidButton.pressed ? Qt.darker(titleColor, 1.35): titleColor
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
                    source: "qrc:/images/media/android.svg"
                    color: androidState ? root.activeColor : root.inactiveColor

                    SequentialAnimation {
                        loops: Animation.Infinite
                        alwaysRunToEnd: true
                        running: root.commandState

                        ScriptAction { script: buttonIcon.color = root.activeColor; }
                        PauseAnimation { duration: 400 }
                        ScriptAction { script: buttonIcon.color = root.inactiveColor; }
                        PauseAnimation { duration: 300 }

                        onStopped: {
                            if (root.androidState) { buttonIcon.color = root.activeColor; }
                            else { buttonIcon.color = root.inactiveColor; }
                            console.log("aap button anim stop");
                        }
                    }
                }

            }
        }

        Text {
            id: androidStr
            width: 240
            anchors {
                top: androidButton.top
                bottom: androidButton.bottom
            }
            text: "Android Auto\nProjection"
            font.family: fontLoader.name
            font.bold: true
            font.pointSize: 20
            color: titleColor
            elide: Text.ElideNone
            wrapMode: Text.NoWrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
