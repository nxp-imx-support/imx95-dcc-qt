/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import Style 1.0
import QtQuick.Effects
import QtQuick.Controls

Item {
    id: root
    property alias text: nameStr.text
    property alias image: icon.source
    required property color color
    required property bool animEnable

    signal pressed()
    signal clicked()
    width: 64
    height: 80

    Button {
        id: button
        anchors.verticalCenter: parent.verticalCenter
        width: 64
        height: 64

        onClicked: root.clicked()
        onPressed: {
            root.pressed();
            if (animEnable) {
                nameAnim.restart();
            }
        }

        background: Column {
            anchors.fill: parent

            ColorIcon {
                id: icon
                height: parent.height * 0.75
                width: parent.width * 0.75
                anchors.horizontalCenter: parent.horizontalCenter
                color: (button.pressed && root.animEnable) ? Qt.darker(root.color, 1.35): root.color
            }

            Text {
                id: nameStr
                width: 64
                height: 16
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0
                PropertyAnimation on opacity {
                    id: nameAnim
                    from: 1
                    to: 0
                    duration: 600
                    easing.type: Easing.InOutQuad
                }
                font.pixelSize: 12
                font.family: "Sarabun"
                color: Qt.darker(root.color, 1.35)
                elide: Text.ElideNone
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
