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

    height: userName.implicitHeight + message.implicitHeight + 8

    required property var modelData

    clip: true

    background: Rectangle {
        id: backgroundBox
        height: parent.height
        width: parent.width
        color: "transparent"
        // opacity: 0.4
        // radius: 12
    }

    contentItem: Item {
        id: infoBox
        height: parent.height
        width: parent.width

        Text {
            id: userName
            property string textContent: swipeBox.modelData.userName
            font.family: fontLoader.name
            font.pointSize: 14
            anchors {
                top: parent.top
                topMargin: 0
                left: modelData.fromSelf ? undefined : avatar.right
                leftMargin: 4
                right: modelData.fromSelf ? avatar.left : undefined
                rightMargin: 4
            }
            width: swipeBox.width * 0.75
            color: Style.grey
            horizontalAlignment: modelData.fromSelf ? Text.AlignRight : Text.AlignLeft
            elide: Text.ElideNone
            wrapMode: Text.NoWrap
            lineHeight: 0.64
            text: {
                if (modelData.isTyping) {
                    return swipeBox.modelData.userName + " " + "...";
                } else {
                    return swipeBox.modelData.userName + " " + swipeBox.modelData.timeStamp.toLocaleTimeString("en_US", "hh:mm");
                }
            }
        }

        Text {
            id: message
            font.family: fontLoader.name
            font.pointSize: 14
            anchors {
                bottom: parent.bottom
                bottomMargin: 4
                left: modelData.fromSelf ? undefined : avatar.right
                leftMargin: 4
                right: modelData.fromSelf ? avatar.left : undefined
                rightMargin: 4
            }
            width: swipeBox.width * 0.75
            color: Style.lightPeriwinkle
            horizontalAlignment: modelData.fromSelf ? Text.AlignRight : Text.AlignLeft
            elide: Text.ElideNone
            wrapMode: Text.Wrap
            lineHeight: 0.64
            text: swipeBox.modelData.message
        }

        TellTalesIndicator
        {
            id: avatar
            height: 32
            width: 32
            source: modelData.fromSelf ? "qrc:/images/telltales/user.svg" : "qrc:/images/telltales/robot.svg"
            activeColor: Style.highlighterGreen
            inactiveColor: "transparent"
            indicatorOpacity: TellTalesModel.indicatorOpacity
            active: true
            blinking: modelData.isTyping
            anchors {
                top: userName.top
                topMargin: 8
                left: modelData.fromSelf ? undefined : parent.left
                leftMargin: 4
                right: modelData.fromSelf ? parent.right : undefined
                rightMargin: 4
            }
        }
    }

//    swipe.right: Row {
//        anchors.left: backgroundBox.right
//        anchors.leftMargin: 6
//        height: parent.height
//        spacing: 4
//
//        Label {
//            id: unpairLabel
//            text: qsTr("Unpair")
//            font.pointSize: 14
//            color: unpairLabel.SwipeDelegate.pressed ?
//                Qt.darker(Style.lightPeriwinkle, 1.35) : Style.lightPeriwinkle
//            verticalAlignment: Label.AlignVCenter
//            horizontalAlignment: Label.AlignHCenter
//            height: parent.height
//            width: 100
//
//            SwipeDelegate.onClicked: {
//                swipeBox.modelData.unpair();
//            }
//
//            background: Rectangle {
//                color: "tomato"
//                opacity: 0.4
//                radius: 12
//            }
//        }
//
//        Rectangle {
//            height: parent.height
//            width: 2
//            color: "transparent"
//        }
//    }

    swipe.transition: Transition {
        SmoothedAnimation { velocity: 3; easing.type: Easing.InOutCubic }
    }
}
