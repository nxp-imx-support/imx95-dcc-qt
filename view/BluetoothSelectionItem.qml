/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Controls
import QtQml.Models
import Style 1.0
import TellTalesModel 1.0

SwipeDelegate {
    id: swipeBox

    required property var modelData

    property color defaultColor: {
        modelData.pairState ? Style.accentBlue : "silver"
    }

    clip: true

    background: Rectangle {
        id: backgroundBox
        height: parent.height
        width: parent.width
        color: (swipeBox.pressed && (swipeBox.swipe.position === 0.0)) ?
            Qt.darker(swipeBox.defaultColor, 1.35) : swipeBox.defaultColor
        opacity: 0.4
        radius: 12
    }

    onClicked: {
        if (swipeBox.modelData.connectionState) {
            swipeBox.modelData.disconnect()
        }
        else {
            swipeBox.modelData.connect()
        }
    }

    contentItem: Item {
        id: infoBox
        height: parent.height
        width: parent.width

        Text {
            id: deviceName
            property string textContent: swipeBox.modelData.deviceName
            font.family: fontLoader.name
            font.pointSize: 20
            anchors.top: parent.top
            anchors.topMargin: -4
            anchors.left: parent.left
            anchors.leftMargin: 48
            anchors.right: parent.right
            anchors.rightMargin: 8
            color: (swipeBox.pressed && (swipeBox.swipe.position === 0.0)) ?
                Qt.darker(Style.lightPeriwinkle, 1.35) : Style.lightPeriwinkle
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideMiddle
            width: parent.width
            wrapMode: Text.NoWrap
            text: textContent
        }

        Text {
            id: deviceAddress
            property string textContent: swipeBox.modelData.deviceAddress
            font.family: fontLoader.name
            font.pointSize: deviceName.font.pointSize * 0.7
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -2
            anchors.left: parent.left
            anchors.leftMargin: 48
            anchors.right: parent.right
            anchors.rightMargin: 8
            color: Style.grey
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideMiddle
            width: parent.width
            wrapMode: Text.NoWrap
            text: textContent
        }

        TellTalesIndicator
        {
            height: 32
            width: 32
            source: "qrc:/images/telltales/bluetooth.svg";
            activeColor: Style.highlighterGreen;
            inactiveColor: "transparent"
            indicatorOpacity: TellTalesModel.indicatorOpacity;
            active: ((!swipeBox.modelData.activeState) && swipeBox.modelData.connectionState);
            blinking: swipeBox.modelData.activeState
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 4
            }
        }
    }

    swipe.right: Row {
        anchors.left: backgroundBox.right
        anchors.leftMargin: 6
        height: parent.height
        spacing: 4

        Label {
            id: unpairLabel
            text: qsTr("Unpair")
            font.pointSize: 14
            color: unpairLabel.SwipeDelegate.pressed ?
                Qt.darker(Style.lightPeriwinkle, 1.35) : Style.lightPeriwinkle
            verticalAlignment: Label.AlignVCenter
            horizontalAlignment: Label.AlignHCenter
            height: parent.height
            width: 100

            SwipeDelegate.onClicked: {
                swipeBox.modelData.unpair();
            }

            background: Rectangle {
                color: "tomato"
                opacity: 0.4
                radius: 12
            }
        }

        Rectangle {
            height: parent.height
            width: 2
            color: "transparent"
        }
    }

    swipe.transition: Transition {
        SmoothedAnimation { velocity: 3; easing.type: Easing.InOutCubic }
    }
}
