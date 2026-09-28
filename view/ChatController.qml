/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Layouts
import Style 1.0

Item {
    id: root

    property alias buttonEnabled: buttonIcon.enabled
    required property bool hintEnabled
    property alias staticColor: buttonIcon.color
    property alias hintText: hintStr.text

    property color titleColor: Qt.darker(Style.lightPeriwinkle, 1.35)
    property color hintColor: Style.orange

    signal chatClicked()

    onHintEnabledChanged: {
        if (root.hintEnabled) {
            hintStrEnableAnim.restart();
        } else {
            hintStrDisableAnim.restart();
        }
    }

    Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }

        Button {
            id: chatButton
            width: 100
            height: 100

            onClicked: root.chatClicked()

            background: Rectangle {
                id: buttonBackground
                radius: width / 2
                color: "transparent"
                border {
                    color: chatButton.pressed ? Qt.darker(titleColor, 1.35) : titleColor
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
                    source: "qrc:/images/telltales/robot.svg"
                }

            }
        }

        ColumnLayout {
            width: 240
            anchors {
                top: chatButton.top
                bottom: chatButton.bottom
            }
            Text {
                id: titleStr
                width: parent.width
                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                text: "Generative AI"
                font.family: fontLoader.name
                font.bold: true
                font.pointSize: 20
                color: root.titleColor
                elide: Text.ElideNone
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                clip: true
            }
            Text {
                id: hintStr
                width: parent.width
                Layout.alignment: Qt.AlignHCenter
                text: ""
                font.family: fontLoader.name
                font.pointSize: titleStr.font.pointSize * 0.6
                color: root.hintColor
                elide: Text.ElideNone
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                clip: true

                Behavior on opacity {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }
                Behavior on Layout.preferredHeight {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                SequentialAnimation {
                    id: hintStrEnableAnim
                    ScriptAction {
                        script: {
                            hintStr.opacity = 1.0;
                            hintStr.Layout.preferredHeight = hintStr.contentHeight;
                        }
                    }
                }
                SequentialAnimation {
                    id: hintStrDisableAnim
                    ScriptAction {
                        script: {
                            hintStr.opacity = 0.0;
                            hintStr.Layout.preferredHeight = 0.0;
                        }
                    }
                }
            }

            Component.onCompleted: {
                if (!root.hintEnabled) {
                    // disable hint
                    hintStr.opacity = 0.0
                    hintStr.Layout.preferredHeight = 0.0;
                    hintStr.visible = false;
                } else {
                    // enable hint
                    hintStr.opacity = 1.0
                    hintStr.Layout.preferredHeight = hintStr.contentHeight;
                    hintStr.visible = true;
                }
            }
        }
    }
}
