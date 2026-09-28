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

    required property bool helmetState0
    required property bool helmetState1
    required property bool linkState
    required property bool commandState

    property color activeColor: Style.highlighterGreen
    property color inactiveColor: Style.darkBlue
    property color unavailableColor: Style.darkRed
    property color titleColor: Qt.darker(Style.lightPeriwinkle, 1.35)
    property color hintColor: Style.orange

    signal helmetClicked()

    function getStaticColor() : color {
        if (!(helmetState0 && helmetState1)) {
            return unavailableColor;
        } else if (linkState) {
            return root.activeColor;
        } else {
            return root.inactiveColor;
        }
    }

    Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }

        Button {
            id: linkButton
            width: 100
            height: 100
            enabled: (helmetState0 && helmetState1)

            onEnabledChanged: {
                console.log("helmet link button", enabled);
                if (enabled) {
                    hintStrDisableAnim.restart();
                } else {
                    hintStrEnableAnim.restart();
                }
            }

            onClicked: root.helmetClicked()

            background: Rectangle {
                id: buttonBackground
                radius: width / 2
                color: "transparent"
                border {
                    color: linkButton.pressed ? Qt.darker(titleColor, 1.35): titleColor
                    width: 5
                }

                ColorIcon {
                    id: buttonIcon
                    height: parent.height * 0.6
                    width: parent.width * 0.6

                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    source: "qrc:/images/media/link.svg"
                    color: root.getStaticColor();

                    SequentialAnimation {
                        loops: Animation.Infinite
                        alwaysRunToEnd: true
                        running: root.commandState

                        ScriptAction { script: buttonIcon.color = root.activeColor; }
                        PauseAnimation { duration: 400 }
                        ScriptAction { script: buttonIcon.color = root.inactiveColor; }
                        PauseAnimation { duration: 300 }

                        onStopped: {
                            buttonIcon.color = root.getStaticColor();
                            console.log("aap button anim stop");
                        }
                    }
                }
            }
        }

        Column {
            width: 40
            anchors {
                verticalCenter: parent.verticalCenter
            }
            spacing: 18

            ColorIcon {
                id: helmetStateIcon0
                width: 36
                height: 36
                anchors {
                    horizontalCenter: parent.horizontalCenter
                }
                source: "qrc:/images/telltales/helmet.svg"
                color: root.helmetState0 ? root.activeColor : root.inactiveColor
            }
            ColorIcon {
                id: helmetStateIcon1
                width: 36
                height: 36
                anchors {
                    horizontalCenter: parent.horizontalCenter
                }
                source: "qrc:/images/telltales/helmet.svg"
                color: root.helmetState1 ? root.activeColor : root.inactiveColor
            }
        }

        ColumnLayout {
            width: 200
            anchors {
                top: linkButton.top
                bottom: linkButton.bottom
            }
            Text {
                id: titleStr
                width: parent.width
                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                text: "Helmet\nInterconnect"
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
                text: "connect both helmets"
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
                if (linkButton.enabled) {
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
